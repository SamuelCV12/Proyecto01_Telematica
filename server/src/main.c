#include "config_loader.h"
#include "logger.h"
#include "nodos.h"
#include "tcp_listener.h"
#include "udp_listener.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

volatile int apagando = 0;

void manejador_senal(int sig) {
  (void)sig;
  apagando = 1;
}

int main(void) {
  logger_init("server.log");
  log_msg(LOG_INFO, "Servidor iniciando...");

  cargar_configuracion("config/server.env");

  signal(SIGINT, manejador_senal);
  signal(SIGTERM, manejador_senal);

  if (!inicializar_tabla_nodos()) {
    log_msg(LOG_ERROR, "El servidor no puede iniciar sin persistencia SQLite");
    logger_close();
    return 1;
  }

  pthread_t hilo_udp_id, hilo_tcp_id;

  if (pthread_create(&hilo_udp_id, NULL, hilo_udp, NULL) != 0) {
    log_msg(LOG_ERROR, "Error al crear hilo UDP");
    logger_close();
    return 1;
  }

  if (pthread_create(&hilo_tcp_id, NULL, hilo_tcp, NULL) != 0) {
    log_msg(LOG_ERROR, "Error al crear hilo TCP");
    apagando = 1;
    pthread_join(hilo_udp_id, NULL);
    logger_close();
    return 1;
  }

  log_msg(LOG_INFO, "Servidor iniciado. Presiona Ctrl+C para detener.");

  while (!apagando) {
    sleep(1);
  }

  log_msg(LOG_INFO, "Señal de apagado recibida, cerrando hilos...");

  pthread_join(hilo_udp_id, NULL);
  pthread_join(hilo_tcp_id, NULL);

  log_msg(LOG_INFO, "Servidor apagado correctamente.");
  cerrar_persistencia();
  logger_close();
  return 0;
}