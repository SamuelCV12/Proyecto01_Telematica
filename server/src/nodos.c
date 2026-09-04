#include "nodos.h"
#include "logger.h"
#include <errno.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define DATABASE_PATH "data/telemetria.db"

NodoTelemetria tabla_nodos[MAX_NODES];
int cantidad_nodos = 0;

Alerta historial_alertas[MAX_ALERTS];
int cantidad_alertas = 0;

pthread_mutex_t mutex_tabla = PTHREAD_MUTEX_INITIALIZER;
time_t hora_inicio_servidor;
static sqlite3 *base_datos = NULL;

static int ejecutar_sql(const char *sql) {
  char *error = NULL;
  int resultado = sqlite3_exec(base_datos, sql, NULL, NULL, &error);
  if (resultado != SQLITE_OK) {
    log_msg(LOG_ERROR, "Error SQLite: %s", error != NULL ? error : "desconocido");
    sqlite3_free(error);
    return 0;
  }
  return 1;
}

static int guardar_nodo(const NodoTelemetria *nodo) {
  const char *sql =
      "INSERT INTO nodos (id, temperatura, humedad, consumo, vibracion, estado, "
      "ultima_actualizacion) VALUES (?, ?, ?, ?, ?, ?, ?) "
      "ON CONFLICT(id) DO UPDATE SET temperatura=excluded.temperatura, "
      "humedad=excluded.humedad, consumo=excluded.consumo, "
      "vibracion=excluded.vibracion, estado=excluded.estado, "
      "ultima_actualizacion=excluded.ultima_actualizacion";
  sqlite3_stmt *sentencia = NULL;
  int resultado = sqlite3_prepare_v2(base_datos, sql, -1, &sentencia, NULL);
  if (resultado == SQLITE_OK) {
    sqlite3_bind_text(sentencia, 1, nodo->id, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(sentencia, 2, nodo->temperatura);
    sqlite3_bind_double(sentencia, 3, nodo->humedad);
    sqlite3_bind_double(sentencia, 4, nodo->consumo_energetico);
    sqlite3_bind_double(sentencia, 5, nodo->vibracion);
    sqlite3_bind_text(sentencia, 6, nodo->estado, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(sentencia, 7, (sqlite3_int64)nodo->ultima_actualizacion);
    resultado = sqlite3_step(sentencia);
  }
  sqlite3_finalize(sentencia);
  if (resultado != SQLITE_DONE) {
    log_msg(LOG_ERROR, "No se pudo guardar el nodo %s en SQLite", nodo->id);
    return 0;
  }
  return 1;
}

static int guardar_alerta(const Alerta *alerta) {
  const char *sql =
      "INSERT INTO alertas (nodo_id, variable, valor, timestamp) VALUES (?, ?, ?, ?)";
  sqlite3_stmt *sentencia = NULL;
  int resultado = sqlite3_prepare_v2(base_datos, sql, -1, &sentencia, NULL);
  if (resultado == SQLITE_OK) {
    sqlite3_bind_text(sentencia, 1, alerta->nodo_id, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(sentencia, 2, alerta->variable, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(sentencia, 3, alerta->valor);
    sqlite3_bind_int64(sentencia, 4, (sqlite3_int64)alerta->timestamp);
    resultado = sqlite3_step(sentencia);
  }
  sqlite3_finalize(sentencia);
  if (resultado != SQLITE_DONE) {
    log_msg(LOG_ERROR, "No se pudo guardar alerta de %s en SQLite", alerta->nodo_id);
    return 0;
  }
  return 1;
}

static void cargar_datos(void) {
  sqlite3_stmt *sentencia = NULL;
  const char *sql_nodos =
      "SELECT id, temperatura, humedad, consumo, vibracion, estado, "
      "ultima_actualizacion FROM nodos ORDER BY id LIMIT ?";
  if (sqlite3_prepare_v2(base_datos, sql_nodos, -1, &sentencia, NULL) == SQLITE_OK) {
    sqlite3_bind_int(sentencia, 1, MAX_NODES);
    while (sqlite3_step(sentencia) == SQLITE_ROW) {
      NodoTelemetria *nodo = &tabla_nodos[cantidad_nodos++];
      snprintf(nodo->id, sizeof(nodo->id), "%s",
               (const char *)sqlite3_column_text(sentencia, 0));
      nodo->temperatura = (float)sqlite3_column_double(sentencia, 1);
      nodo->humedad = (float)sqlite3_column_double(sentencia, 2);
      nodo->consumo_energetico = (float)sqlite3_column_double(sentencia, 3);
      nodo->vibracion = (float)sqlite3_column_double(sentencia, 4);
      snprintf(nodo->estado, sizeof(nodo->estado), "%s",
               (const char *)sqlite3_column_text(sentencia, 5));
      nodo->ultima_actualizacion = (time_t)sqlite3_column_int64(sentencia, 6);
    }
  }
  sqlite3_finalize(sentencia);

  const char *sql_alertas =
      "SELECT nodo_id, variable, valor, timestamp FROM alertas "
      "ORDER BY timestamp DESC LIMIT ?";
  if (sqlite3_prepare_v2(base_datos, sql_alertas, -1, &sentencia, NULL) == SQLITE_OK) {
    sqlite3_bind_int(sentencia, 1, MAX_ALERTS);
    while (sqlite3_step(sentencia) == SQLITE_ROW) {
      Alerta *alerta = &historial_alertas[cantidad_alertas++];
      snprintf(alerta->nodo_id, sizeof(alerta->nodo_id), "%s",
               (const char *)sqlite3_column_text(sentencia, 0));
      snprintf(alerta->variable, sizeof(alerta->variable), "%s",
               (const char *)sqlite3_column_text(sentencia, 1));
      alerta->valor = (float)sqlite3_column_double(sentencia, 2);
      alerta->timestamp = (time_t)sqlite3_column_int64(sentencia, 3);
    }
  }
  sqlite3_finalize(sentencia);
}

int inicializar_tabla_nodos(void) {
  pthread_mutex_lock(&mutex_tabla);
  memset(tabla_nodos, 0, sizeof(tabla_nodos));
  cantidad_nodos = 0;
  memset(historial_alertas, 0, sizeof(historial_alertas));
  cantidad_alertas = 0;
  hora_inicio_servidor = time(NULL);
  if (mkdir("data", 0755) < 0 && errno != EEXIST) {
    log_msg(LOG_ERROR, "No se pudo crear el directorio de datos");
    pthread_mutex_unlock(&mutex_tabla);
    return 0;
  }
  if (sqlite3_open(DATABASE_PATH, &base_datos) != SQLITE_OK ||
      !ejecutar_sql("CREATE TABLE IF NOT EXISTS nodos ("
                    "id TEXT PRIMARY KEY, temperatura REAL NOT NULL DEFAULT 0,"
                    "humedad REAL NOT NULL DEFAULT 0, consumo REAL NOT NULL DEFAULT 0,"
                    "vibracion REAL NOT NULL DEFAULT 0, estado TEXT NOT NULL,"
                    "ultima_actualizacion INTEGER NOT NULL)") ||
      !ejecutar_sql("CREATE TABLE IF NOT EXISTS alertas ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, nodo_id TEXT NOT NULL,"
                    "variable TEXT NOT NULL, valor REAL NOT NULL, timestamp INTEGER NOT NULL)")) {
    log_msg(LOG_ERROR, "No se pudo inicializar la persistencia SQLite en %s",
            DATABASE_PATH);
    if (base_datos != NULL) {
      sqlite3_close(base_datos);
      base_datos = NULL;
    }
    pthread_mutex_unlock(&mutex_tabla);
    return 0;
  }
  cargar_datos();
  pthread_mutex_unlock(&mutex_tabla);
  log_msg(LOG_INFO, "Persistencia SQLite inicializada en %s", DATABASE_PATH);
  return 1;
}

void cerrar_persistencia(void) {
  pthread_mutex_lock(&mutex_tabla);
  if (base_datos != NULL) {
    sqlite3_close(base_datos);
    base_datos = NULL;
  }
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
  if (base_datos != NULL) {
    guardar_alerta(destino);
  }
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
  if (base_datos != NULL) {
    guardar_nodo(nodo);
  }

  pthread_mutex_unlock(&mutex_tabla);
}