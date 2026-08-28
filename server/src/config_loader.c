#include "config_loader.h"
#include "config.h"
#include "logger.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int server_udp_port;
int server_tcp_port;
char server_host[128];

static void quitar_espacios(char *s) {
  // Quita espacios/tabs al inicio
  size_t inicio = 0;
  while (s[inicio] != '\0' && isspace((unsigned char)s[inicio])) {
    inicio++;
  }
  if (inicio > 0) {
    memmove(s, s + inicio, strlen(s + inicio) + 1);
  }

  // Quita \r \n espacios al final
  size_t len = strlen(s);
  while (len > 0 && isspace((unsigned char)s[len - 1])) {
    s[len - 1] = '\0';
    len--;
  }
}

void cargar_configuracion(const char *ruta_archivo) {
  server_udp_port = DEFAULT_UDP_PORT;
  server_tcp_port = DEFAULT_TCP_PORT;
  strncpy(server_host, "localhost", sizeof(server_host) - 1);
  server_host[sizeof(server_host) - 1] = '\0';

  FILE *archivo = fopen(ruta_archivo, "r");
  if (archivo == NULL) {
    log_msg(LOG_WARN, "No se encontró %s, usando valores por defecto",
            ruta_archivo);
    return;
  }

  char linea[256];
  while (fgets(linea, sizeof(linea), archivo) != NULL) {
    quitar_espacios(linea);

    if (linea[0] == '\0' || linea[0] == '#') {
      continue;
    }

    char *separador = strchr(linea, '=');
    if (separador == NULL)
      continue;

    *separador = '\0';
    char *clave = linea;
    char *valor = separador + 1;

    if (strcmp(clave, "SERVER_UDP_PORT") == 0) {
      server_udp_port = atoi(valor);
    } else if (strcmp(clave, "SERVER_TCP_PORT") == 0) {
      server_tcp_port = atoi(valor);
    } else if (strcmp(clave, "SERVER_HOST") == 0) {
      strncpy(server_host, valor, sizeof(server_host) - 1);
      server_host[sizeof(server_host) - 1] = '\0';
    }
  }

  fclose(archivo);

  log_msg(LOG_INFO, "Configuración cargada desde %s (UDP=%d, TCP=%d, HOST=%s)",
          ruta_archivo, server_udp_port, server_tcp_port, server_host);
}