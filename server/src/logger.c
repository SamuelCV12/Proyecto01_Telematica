#include "logger.h"
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static FILE *archivo_log = NULL;
static pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;

void logger_init(const char *ruta_archivo) {
  setvbuf(stdout, NULL, _IONBF, 0); // desactiva el buffering de stdout

  archivo_log = fopen(ruta_archivo, "a");
  if (archivo_log == NULL) {
    perror("[LOGGER] no se pudo abrir el archivo de log");
  }
}

void logger_close(void) {
  if (archivo_log != NULL) {
    fclose(archivo_log);
    archivo_log = NULL;
  }
}

static const char *nivel_str(NivelLog nivel) {
  switch (nivel) {
  case LOG_INFO:
    return "INFO";
  case LOG_WARN:
    return "WARN";
  case LOG_ERROR:
    return "ERROR";
  default:
    return "?";
  }
}

void log_msg(NivelLog nivel, const char *formato, ...) {
  pthread_mutex_lock(&mutex_log);

  time_t ahora = time(NULL);
  struct tm *t = localtime(&ahora);
  char marca_tiempo[32];
  strftime(marca_tiempo, sizeof(marca_tiempo), "%Y-%m-%d %H:%M:%S", t);

  va_list args1, args2;
  va_start(args1, formato);
  va_copy(args2, args1);

  printf("[%s] [%s] ", marca_tiempo, nivel_str(nivel));
  vprintf(formato, args1);
  printf("\n");
  va_end(args1);

  if (archivo_log != NULL) {
    fprintf(archivo_log, "[%s] [%s] ", marca_tiempo, nivel_str(nivel));
    vfprintf(archivo_log, formato, args2);
    fprintf(archivo_log, "\n");
    fflush(archivo_log);
  }
  va_end(args2);

  pthread_mutex_unlock(&mutex_log);
}