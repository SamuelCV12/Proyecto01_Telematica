#include "udp_listener.h"
#include "config.h"
#include "config_loader.h"
#include "logger.h"
#include "nodos.h"
#include <arpa/inet.h>
#include <errno.h>
#include <math.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

extern volatile int apagando;

static int convertir_valor(const char *texto, float *valor) {
  char *fin = NULL;

  if (texto == NULL || *texto == '\0') {
    return 0;
  }

  errno = 0;
  float convertido = strtof(texto, &fin);
  if (errno == ERANGE || fin == texto || *fin != '\0' ||
      !isfinite(convertido)) {
    return 0;
  }

  *valor = convertido;
  return 1;
}

static int variable_valida(const char *variable) {
  return strcmp(variable, "TEMP") == 0 || strcmp(variable, "HUM") == 0 ||
         strcmp(variable, "CONSUMO") == 0 ||
         strcmp(variable, "VIBRACION") == 0;
}

static int valor_valido(const char *variable, float valor) {
  if (strcmp(variable, "TEMP") == 0) {
    return valor >= -50.0f && valor <= 100.0f;
  }
  if (strcmp(variable, "HUM") == 0) {
    return valor >= 0.0f && valor <= 100.0f;
  }
  if (strcmp(variable, "CONSUMO") == 0 ||
      strcmp(variable, "VIBRACION") == 0) {
    return valor >= 0.0f;
  }
  return 0;
}

static int alerta_valida(const char *codigo) {
  return strcmp(codigo, "TEMP_HIGH") == 0 ||
         strcmp(codigo, "HUM_HIGH") == 0 ||
         strcmp(codigo, "CONSUMO_HIGH") == 0 ||
         strcmp(codigo, "VIBRACION_HIGH") == 0;
}

static int id_nodo_valido(const char *id) {
  if (id == NULL || strlen(id) >= sizeof(((NodoTelemetria *)0)->id) ||
      strncmp(id, "NODE", 4) != 0 || id[4] == '\0') {
    return 0;
  }

  for (const char *caracter = id + 4; *caracter != '\0'; caracter++) {
    if (*caracter < '0' || *caracter > '9') {
      return 0;
    }
  }
  return 1;
}

static void procesar_mensaje(char *buffer) {
  log_msg(LOG_INFO, "UDP recibido: %s", buffer);

  char *tipo = strtok(buffer, "|");
  if (tipo == NULL) {
    log_msg(LOG_WARN, "Mensaje UDP con tipo desconocido, descartado");
    return;
  }

  char *id = strtok(NULL, "|");
  if (id == NULL) {
    log_msg(LOG_WARN, "Mensaje UDP mal formado, descartado");
    return;
  }
  if (!id_nodo_valido(id)) {
    log_msg(LOG_WARN, "ERR_02: identificador de nodo no autorizado: %s", id);
    return;
  }

  if (strcmp(tipo, "TELEMETRY") == 0) {
    char *variable = strtok(NULL, "|");
    char *valor_str = strtok(NULL, "|");

    if (variable == NULL || valor_str == NULL) {
      log_msg(LOG_WARN, "Mensaje TELEMETRY mal formado, descartado");
      return;
    }

    if (strtok(NULL, "|") != NULL) {
      log_msg(LOG_WARN, "ERR_01: TELEMETRY contiene campos adicionales");
      return;
    }

    float valor;
    if (!variable_valida(variable)) {
      log_msg(LOG_WARN, "ERR_01: variable TELEMETRY desconocida: %s", variable);
      return;
    }
    if (!convertir_valor(valor_str, &valor)) {
      log_msg(LOG_WARN, "ERR_01: valor TELEMETRY no numérico: %s", valor_str);
      return;
    }
    if (!valor_valido(variable, valor)) {
      log_msg(LOG_WARN, "ERR_03: valor TELEMETRY fuera de rango: %s=%.2f",
              variable, valor);
      return;
    }

    actualizar_medicion(id, variable, valor);
    log_msg(LOG_INFO, "Nodo %s actualizado: %s=%.2f", id, variable, valor);
  } else if (strcmp(tipo, "ALERT") == 0) {
    char *codigo = strtok(NULL, "|");
    char *valor_str = strtok(NULL, "|");

    if (codigo == NULL || valor_str == NULL) {
      log_msg(LOG_WARN, "Mensaje ALERT mal formado, descartado");
      return;
    }

    if (strtok(NULL, "|") != NULL) {
      log_msg(LOG_WARN, "ERR_01: ALERT contiene campos adicionales");
      return;
    }

    float valor;
    if (!alerta_valida(codigo)) {
      log_msg(LOG_WARN, "ERR_01: código ALERT desconocido: %s", codigo);
      return;
    }
    if (!convertir_valor(valor_str, &valor)) {
      log_msg(LOG_WARN, "ERR_01: valor ALERT no numérico: %s", valor_str);
      return;
    }
    if (valor < 0.0f) {
      log_msg(LOG_WARN, "ERR_03: valor ALERT fuera de rango: %s=%.2f", codigo,
              valor);
      return;
    }

    registrar_alerta(id, codigo, valor);
    log_msg(LOG_WARN, "Alerta recibida de %s: %s=%.2f", id, codigo, valor);
  } else {
    log_msg(LOG_WARN, "Mensaje UDP con tipo desconocido, descartado");
  }
}

void *hilo_udp(void *arg) {
  (void)arg;

  int sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock_udp < 0) {
    log_msg(LOG_ERROR, "Error al crear socket UDP");
    return NULL;
  }

  struct sockaddr_in direccion;
  memset(&direccion, 0, sizeof(direccion));
  direccion.sin_family = AF_INET;
  direccion.sin_addr.s_addr = INADDR_ANY;
  direccion.sin_port = htons(server_udp_port);

  if (bind(sock_udp, (struct sockaddr *)&direccion, sizeof(direccion)) < 0) {
    log_msg(LOG_ERROR, "Error en bind UDP");
    close(sock_udp);
    return NULL;
  }

  log_msg(LOG_INFO, "UDP escuchando en puerto %d", server_udp_port);

  char buffer[BUFFER_SIZE];
  struct sockaddr_in cliente;
  socklen_t cliente_len = sizeof(cliente);

  while (!apagando) {
    fd_set lectura;
    FD_ZERO(&lectura);
    FD_SET(sock_udp, &lectura);

    struct timeval timeout;
    timeout.tv_sec = SELECT_TIMEOUT_SEC;
    timeout.tv_usec = 0;

    int listo = select(sock_udp + 1, &lectura, NULL, NULL, &timeout);

    if (listo < 0) {
      if (apagando)
        break;
      log_msg(LOG_ERROR, "Error en select (UDP)");
      continue;
    }

    if (listo == 0) {
      continue;
    }

    memset(buffer, 0, BUFFER_SIZE);
    ssize_t bytes = recvfrom(sock_udp, buffer, BUFFER_SIZE - 1, 0,
                             (struct sockaddr *)&cliente, &cliente_len);

    if (bytes < 0) {
      log_msg(LOG_ERROR, "Error en recvfrom (UDP)");
      continue;
    }

    buffer[bytes] = '\0';
    procesar_mensaje(buffer);
  }

  close(sock_udp);
  log_msg(LOG_INFO, "Hilo UDP finalizado");
  return NULL;
}