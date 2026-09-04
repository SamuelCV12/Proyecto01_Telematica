#include "udp_listener.h"
#include "config.h"
#include "config_loader.h"
#include "logger.h"
#include "nodos.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

extern volatile int apagando;

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

  if (strcmp(tipo, "TELEMETRY") == 0) {
    char *variable = strtok(NULL, "|");
    char *valor_str = strtok(NULL, "|");

    if (variable == NULL || valor_str == NULL) {
      log_msg(LOG_WARN, "Mensaje TELEMETRY mal formado, descartado");
      return;
    }

    float valor = atof(valor_str);
    actualizar_medicion(id, variable, valor);
    log_msg(LOG_INFO, "Nodo %s actualizado: %s=%.2f", id, variable, valor);
  } else if (strcmp(tipo, "ALERT") == 0) {
    char *codigo = strtok(NULL, "|");
    char *valor_str = strtok(NULL, "|");

    if (codigo == NULL || valor_str == NULL) {
      log_msg(LOG_WARN, "Mensaje ALERT mal formado, descartado");
      return;
    }

    float valor = atof(valor_str);
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