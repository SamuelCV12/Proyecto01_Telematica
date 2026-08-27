#ifndef NODOS_H
#define NODOS_H

#include "config.h"
#include <pthread.h>
#include <time.h>

typedef struct {
  char id[32];
  float temperatura;
  float humedad;
  float consumo_energetico;
  float vibracion;
  char estado[16];
  time_t ultima_actualizacion;
  int activo;
} NodoTelemetria;

extern NodoTelemetria tabla_nodos[MAX_NODES];
extern int cantidad_nodos;
extern pthread_mutex_t mutex_tabla;

void inicializar_tabla_nodos(void);
void actualizar_medicion(const char *id, const char *variable, float valor);

#endif