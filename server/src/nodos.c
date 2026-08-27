#include "nodos.h"
#include "logger.h"
#include <string.h>
#include <time.h>
#include <stdlib.h>

NodoTelemetria tabla_nodos[MAX_NODES];
int cantidad_nodos = 0;
pthread_mutex_t mutex_tabla = PTHREAD_MUTEX_INITIALIZER;

void inicializar_tabla_nodos(void) {
  pthread_mutex_lock(&mutex_tabla);
  memset(tabla_nodos, 0, sizeof(tabla_nodos));
  cantidad_nodos = 0;
  pthread_mutex_unlock(&mutex_tabla);
}

static NodoTelemetria *buscar_o_crear_nodo(const char *id) {
  // Asume que el mutex ya está bloqueado por quien llama

  for (int i = 0; i < cantidad_nodos; i++) {
    if (strcmp(tabla_nodos[i].id, id) == 0) {
      return &tabla_nodos[i];
    }
  }

  if (cantidad_nodos < MAX_NODES) {
    NodoTelemetria *nuevo = &tabla_nodos[cantidad_nodos];
    strncpy(nuevo->id, id, sizeof(nuevo->id) - 1);
    nuevo->id[sizeof(nuevo->id) - 1] = '\0';
    strncpy(nuevo->estado, "OK", sizeof(nuevo->estado) - 1);
    nuevo->estado[sizeof(nuevo->estado) - 1] = '\0';
    nuevo->activo = 1;
    nuevo->ultima_actualizacion = time(NULL);
    cantidad_nodos++;
    return nuevo;
  }

  return NULL;
}

void actualizar_medicion(const char *id, const char *variable, float valor) {
  pthread_mutex_lock(&mutex_tabla);

  NodoTelemetria *nodo = buscar_o_crear_nodo(id);
  if (nodo == NULL) {
    // TODO: loguear "tabla de nodos llena, no se pudo registrar"
    pthread_mutex_unlock(&mutex_tabla);
    return;
  }

  if (strcmp(variable, "TEMP") == 0) {
    nodo->temperatura = valor;
    if (valor >= UMBRAL_TEMP_ALTA) {
      strncpy(nodo->estado, "ALERTA", sizeof(nodo->estado) - 1);
    } else {
      strncpy(nodo->estado, "OK", sizeof(nodo->estado) - 1);
    }
    nodo->estado[sizeof(nodo->estado) - 1] = '\0';
  } else if (strcmp(variable, "HUM") == 0) {
    nodo->humedad = valor;
  } else if (strcmp(variable, "CONSUMO") == 0) {
    nodo->consumo_energetico = valor;
  } else if (strcmp(variable, "VIBRACION") == 0) {
    nodo->vibracion = valor;
  } else {
    log_msg(LOG_WARN, "Variable desconocida recibida de nodo %s: %s", id, variable);
  }

  nodo->ultima_actualizacion = time(NULL);
  nodo->activo = 1;

  pthread_mutex_unlock(&mutex_tabla);
}