#include "tcp_listener.h"
#include "config.h"
#include "config_loader.h"
#include "logger.h"
#include "nodos.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

extern volatile int apagando;

typedef struct {
  int socket_cliente;
} ArgOperador;

static void construir_respuesta_status(const char *id, char *respuesta,
                                       size_t tam) {
  pthread_mutex_lock(&mutex_tabla);

  NodoTelemetria *nodo = NULL;
  for (int i = 0; i < cantidad_nodos; i++) {
    if (strcmp(tabla_nodos[i].id, id) == 0) {
      nodo = &tabla_nodos[i];
      break;
    }
  }

  if (nodo == NULL) {
    snprintf(respuesta, tam, "ERROR|NODE_NOT_FOUND|%s\n", id);
  } else {
    const char *actividad = nodo_esta_activo(nodo) ? "ACTIVO" : "INACTIVO";
    snprintf(respuesta, tam,
             "STATUS|%s|TEMP|%.2f|HUM|%.2f|CONSUMO|%.2f|VIBRACION|%.2f|ESTADO|%"
             "s|CONEXION|%s\n",
             nodo->id, nodo->temperatura, nodo->humedad,
             nodo->consumo_energetico, nodo->vibracion, nodo->estado,
             actividad);
  }

  pthread_mutex_unlock(&mutex_tabla);
}

static void construir_respuesta_lista(char *respuesta, size_t tam) {
  pthread_mutex_lock(&mutex_tabla);

  int activos = contar_nodos_activos();

  snprintf(respuesta, tam, "NODES|%d", activos);
  for (int i = 0; i < cantidad_nodos; i++) {
    if (nodo_esta_activo(&tabla_nodos[i])) {
      char item[48];
      snprintf(item, sizeof(item), "|%s", tabla_nodos[i].id);
      strncat(respuesta, item, tam - strlen(respuesta) - 1);
    }
  }
  strncat(respuesta, "\n", tam - strlen(respuesta) - 1);

  pthread_mutex_unlock(&mutex_tabla);
}

static void construir_respuesta_alertas(char *respuesta, size_t tam) {
  pthread_mutex_lock(&mutex_tabla);

  snprintf(respuesta, tam, "ALERTS|%d", cantidad_alertas);
  for (int i = 0; i < cantidad_alertas; i++) {
    char item[64];
    snprintf(item, sizeof(item), "|%s:%s:%.2f:%ld",
             historial_alertas[i].nodo_id, historial_alertas[i].variable,
             historial_alertas[i].valor, (long)historial_alertas[i].timestamp);
    if (strlen(respuesta) + strlen(item) < tam - 2) {
      strncat(respuesta, item, tam - strlen(respuesta) - 1);
    } else {
      break;
    }
  }
  strncat(respuesta, "\n", tam - strlen(respuesta) - 1);

  pthread_mutex_unlock(&mutex_tabla);
}

static void construir_respuesta_system_status(char *respuesta, size_t tam) {
  pthread_mutex_lock(&mutex_tabla);

  int activos = contar_nodos_activos();
  long uptime_seg = (long)(time(NULL) - hora_inicio_servidor);

  snprintf(respuesta, tam,
           "SYSTEM_STATUS|NODOS_TOTAL|%d|NODOS_ACTIVOS|%d|ALERTAS_TOTAL|%d|"
           "UPTIME_SEG|%ld\n",
           cantidad_nodos, activos, cantidad_alertas, uptime_seg);

  pthread_mutex_unlock(&mutex_tabla);
}

void *atender_operador(void *arg) {
  ArgOperador *info = (ArgOperador *)arg;
  int sock = info->socket_cliente;
  free(info);

  char buffer[BUFFER_SIZE];
  char respuesta[BUFFER_SIZE];

  ssize_t bytes = recv(sock, buffer, BUFFER_SIZE - 1, 0);
  if (bytes <= 0) {
    log_msg(LOG_WARN, "Operador desconectado sin enviar datos");
    close(sock);
    return NULL;
  }
  buffer[bytes] = '\0';
  buffer[strcspn(buffer, "\r\n")] = '\0';

  log_msg(LOG_INFO, "TCP recibido: %s", buffer);

  char *tipo = strtok(buffer, "|");

  if (tipo != NULL && strcmp(tipo, "GET_STATUS") == 0) {
    char *id = strtok(NULL, "|");
    if (id != NULL) {
      construir_respuesta_status(id, respuesta, sizeof(respuesta));
    } else {
      snprintf(respuesta, sizeof(respuesta), "ERROR|MISSING_NODE_ID\n");
    }
  } else if (tipo != NULL && strcmp(tipo, "LIST_NODES") == 0) {
    construir_respuesta_lista(respuesta, sizeof(respuesta));
  } else if (tipo != NULL && strcmp(tipo, "GET_ALERTS") == 0) {
    construir_respuesta_alertas(respuesta, sizeof(respuesta));
  } else if (tipo != NULL && strcmp(tipo, "GET_SYSTEM_STATUS") == 0) {
    construir_respuesta_system_status(respuesta, sizeof(respuesta));
  } else {
    snprintf(respuesta, sizeof(respuesta), "ERROR|UNKNOWN_COMMAND\n");
    log_msg(LOG_WARN, "Comando TCP desconocido recibido");
  }

  log_msg(LOG_INFO, "TCP enviado: %s", respuesta);
  send(sock, respuesta, strlen(respuesta), 0);
  close(sock);
  return NULL;
}

void *hilo_tcp(void *arg) {
  (void)arg;

  int sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_tcp < 0) {
    log_msg(LOG_ERROR, "Error al crear socket TCP");
    return NULL;
  }

  int opt = 1;
  setsockopt(sock_tcp, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in direccion;
  memset(&direccion, 0, sizeof(direccion));
  direccion.sin_family = AF_INET;
  direccion.sin_addr.s_addr = INADDR_ANY;
  direccion.sin_port = htons(server_tcp_port);

  if (bind(sock_tcp, (struct sockaddr *)&direccion, sizeof(direccion)) < 0) {
    log_msg(LOG_ERROR, "Error en bind TCP");
    close(sock_tcp);
    return NULL;
  }

  if (listen(sock_tcp, 10) < 0) {
    log_msg(LOG_ERROR, "Error en listen TCP");
    close(sock_tcp);
    return NULL;
  }

  log_msg(LOG_INFO, "TCP escuchando en puerto %d", server_tcp_port);

  while (!apagando) {
    fd_set lectura;
    FD_ZERO(&lectura);
    FD_SET(sock_tcp, &lectura);

    struct timeval timeout;
    timeout.tv_sec = SELECT_TIMEOUT_SEC;
    timeout.tv_usec = 0;

    int listo = select(sock_tcp + 1, &lectura, NULL, NULL, &timeout);

    if (listo < 0) {
      if (apagando)
        break;
      log_msg(LOG_ERROR, "Error en select (TCP)");
      continue;
    }

    if (listo == 0) {
      continue;
    }

    struct sockaddr_in cliente;
    socklen_t cliente_len = sizeof(cliente);
    int sock_cliente =
        accept(sock_tcp, (struct sockaddr *)&cliente, &cliente_len);

    if (sock_cliente < 0) {
      log_msg(LOG_ERROR, "Error en accept (TCP)");
      continue;
    }

    ArgOperador *info = malloc(sizeof(ArgOperador));
    info->socket_cliente = sock_cliente;

    pthread_t hilo_operador;
    if (pthread_create(&hilo_operador, NULL, atender_operador, info) != 0) {
      log_msg(LOG_ERROR, "Error al crear hilo de operador");
      close(sock_cliente);
      free(info);
      continue;
    }
    pthread_detach(hilo_operador);
  }

  close(sock_tcp);
  log_msg(LOG_INFO, "Hilo TCP finalizado");
  return NULL;
}