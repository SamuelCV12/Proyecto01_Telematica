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
} NodoTelemetria;

typedef struct {
  char nodo_id[32];
  char variable[16];
  float valor;
  time_t timestamp;
} Alerta;

extern NodoTelemetria tabla_nodos[MAX_NODES];
extern int cantidad_nodos;

extern Alerta historial_alertas[MAX_ALERTS];
extern int cantidad_alertas;

extern pthread_mutex_t mutex_tabla;
extern time_t hora_inicio_servidor;

void inicializar_tabla_nodos(void);
NodoTelemetria *buscar_o_crear_nodo(const char *id);
void actualizar_medicion(const char *id, const char *variable, float valor);
void registrar_alerta(const char *id, const char *variable, float valor);
int nodo_esta_activo(const NodoTelemetria *nodo);
int contar_nodos_activos(void);

#endif