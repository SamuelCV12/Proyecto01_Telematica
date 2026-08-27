#ifndef LOGGER_H
#define LOGGER_H

typedef enum { LOG_INFO, LOG_WARN, LOG_ERROR } NivelLog;

void logger_init(const char *ruta_archivo);
void logger_close(void);
void log_msg(NivelLog nivel, const char *formato, ...);

#endif