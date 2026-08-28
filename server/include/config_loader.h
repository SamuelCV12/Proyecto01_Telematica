#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

extern int server_udp_port;
extern int server_tcp_port;
extern char server_host[128];

void cargar_configuracion(const char *ruta_archivo);

#endif