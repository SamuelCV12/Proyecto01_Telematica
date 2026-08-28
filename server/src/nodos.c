#include "nodos.h"
#include <string.h>

NodoTelemetria tabla_nodos[MAX_NODES];
int cantidad_nodos = 0;

Alerta historial_alertas[MAX_ALERTS];
int cantidad_alertas = 0;

pthread_mutex_t mutex_tabla = PTHREAD_MUTEX_INITIALIZER;
time_t hora_inicio_servidor;

void inicializar_tabla_nodos(void) {
  pthread_mutex_lock(&mutex_tabla);
  memset(tabla_nodos, 0, sizeof(tabla_nodos));
  cantidad_nodos = 0;
  memset(historial_alertas, 0, sizeof(historial_alertas));
  cantidad_alertas = 0;
  hora_inicio_servidor = time(NULL);
  pthread_mutex_unlock(&mutex_tabla);
}

NodoTelemetria *buscar_o_crear_nodo(const char *id) {
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
    nuevo->ultima_actualizacion = time(NULL);
    cantidad_nodos++;
    return nuevo;
  }

  return NULL;
}

int nodo_esta_activo(const NodoTelemetria *nodo) {
  time_t ahora = time(NULL);
  return (ahora - nodo->ultima_actualizacion) <= TIMEOUT_INACTIVO_SEC;
}

int contar_nodos_activos(void) {
  // Asume que el mutex ya está bloqueado por quien llama
  int activos = 0;
  for (int i = 0; i < cantidad_nodos; i++) {
    if (nodo_esta_activo(&tabla_nodos[i]))
      activos++;
  }
  return activos;
}

void registrar_alerta(const char *id, const char *variable, float valor) {
  // Asume que el mutex ya está bloqueado por quien llama

  Alerta *destino;

  if (cantidad_alertas < MAX_ALERTS) {
    destino = &historial_alertas[cantidad_alertas];
    cantidad_alertas++;
  } else {
    memmove(&historial_alertas[0], &historial_alertas[1],
            sizeof(Alerta) * (MAX_ALERTS - 1));
    destino = &historial_alertas[MAX_ALERTS - 1];
  }

  strncpy(destino->nodo_id, id, sizeof(destino->nodo_id) - 1);
  destino->nodo_id[sizeof(destino->nodo_id) - 1] = '\0';
  strncpy(destino->variable, variable, sizeof(destino->variable) - 1);
  destino->variable[sizeof(destino->variable) - 1] = '\0';
  destino->valor = valor;
  destino->timestamp = time(NULL);
}

static void actualizar_estado_nodo(NodoTelemetria *nodo, int hay_anomalia) {
  if (hay_anomalia) {
    strncpy(nodo->estado, "ALERTA", sizeof(nodo->estado) - 1);
  } else {
    strncpy(nodo->estado, "OK", sizeof(nodo->estado) - 1);
  }
  nodo->estado[sizeof(nodo->estado) - 1] = '\0';
}

void actualizar_medicion(const char *id, const char *variable, float valor) {
  pthread_mutex_lock(&mutex_tabla);

  NodoTelemetria *nodo = buscar_o_crear_nodo(id);
  if (nodo == NULL) {
    pthread_mutex_unlock(&mutex_tabla);
    return;
  }

  if (strcmp(variable, "TEMP") == 0) {
    nodo->temperatura = valor;
    int anomalia = valor >= UMBRAL_TEMP_ALTA;
    actualizar_estado_nodo(nodo, anomalia);
    if (anomalia)
      registrar_alerta(id, variable, valor);
  } else if (strcmp(variable, "HUM") == 0) {
    nodo->humedad = valor;
    int anomalia = valor >= UMBRAL_HUM_ALTA;
    actualizar_estado_nodo(nodo, anomalia);
    if (anomalia)
      registrar_alerta(id, variable, valor);
  } else if (strcmp(variable, "CONSUMO") == 0) {
    nodo->consumo_energetico = valor;
    int anomalia = valor >= UMBRAL_CONSUMO_ALTA;
    actualizar_estado_nodo(nodo, anomalia);
    if (anomalia)
      registrar_alerta(id, variable, valor);
  } else if (strcmp(variable, "VIBRACION") == 0) {
    nodo->vibracion = valor;
    int anomalia = valor >= UMBRAL_VIBRACION_ALTA;
    actualizar_estado_nodo(nodo, anomalia);
    if (anomalia)
      registrar_alerta(id, variable, valor);
  }

  nodo->ultima_actualizacion = time(NULL);

  pthread_mutex_unlock(&mutex_tabla);
}