#include "funcionesDN.h"

TdataNode *obtenerConfiguracionDN(char* ruta){
	TdataNode *pDataNode = malloc(sizeof(TdataNode));

	pDataNode->ip_filesystem       =    malloc(MAXIMA_LONGITUD_IP);
	pDataNode->ip_nodo       =    malloc(MAXIMA_LONGITUD_IP);
	pDataNode->puerto_entrada = malloc(MAXIMA_LONGITUD_PUERTO);
	pDataNode->puerto_master = malloc(MAXIMA_LONGITUD_PUERTO);
	pDataNode->puerto_worker = malloc(MAXIMA_LONGITUD_PUERTO);
	pDataNode->puerto_filesystem = malloc(MAXIMA_LONGITUD_PUERTO);
	pDataNode->ruta_databin=malloc(MAXIMA_LONGITUD_RUTA);
	pDataNode->nombre_nodo=malloc(MAXIMA_LONGITUD_RUTA);

	t_config *dataNodeConfig = config_create(ruta);

	strcpy(pDataNode->ip_filesystem, config_get_string_value(dataNodeConfig, "IP_FILESYSTEM"));
	strcpy(pDataNode->ip_nodo, config_get_string_value(dataNodeConfig, "IP_NODO"));
	strcpy(pDataNode->puerto_entrada, config_get_string_value(dataNodeConfig, "PUERTO_DATANODE"));
	strcpy(pDataNode->puerto_worker, config_get_string_value(dataNodeConfig, "PUERTO_WORKER"));
	strcpy(pDataNode->puerto_master, config_get_string_value(dataNodeConfig, "PUERTO_MASTER"));
	strcpy(pDataNode->puerto_filesystem, config_get_string_value(dataNodeConfig, "PUERTO_FILESYSTEM"));
	strcpy(pDataNode->ruta_databin, config_get_string_value(dataNodeConfig, "RUTA_DATABIN"));
	strcpy(pDataNode->nombre_nodo, config_get_string_value(dataNodeConfig, "NOMBRE_NODO"));
	pDataNode->tamanio_databin_mb = config_get_int_value(dataNodeConfig,"TAMANIO_DATABIN_MB");

	pDataNode->tipo_de_proceso = DATANODE;

	config_destroy(dataNodeConfig);
	return pDataNode;
}

void mostrarConfiguracion(TdataNode *dn){

	printf("Puerto Entrada: %s\n",  dn->puerto_entrada);
	printf("IP Filesystem %s\n",    dn->ip_filesystem);
	printf("IP Nodo %s\n",    dn->ip_nodo);
	printf("Puerto Master: %s\n",       dn->puerto_master);
	printf("Puerto Filesystem: %s\n", dn->puerto_filesystem);
	printf("Puerto Worker: %s\n", dn->puerto_worker);
	printf("Ruta Databin: %s\n", dn->ruta_databin);
	printf("Nombre Nodo: %s\n", dn->nombre_nodo);
	printf("Tamanio databin en MB: %d\n", dn->tamanio_databin_mb);
	printf("Tipo de proceso: %d\n", dn->tipo_de_proceso);
}

void setBloque(int posicion, Tbloque * bloque){
	unsigned long long bloqueSize = BLOQUE_SIZE;
	char *aux = archivoMapeado;

	memcpy(aux += posicion* bloqueSize, bloque->contenido,bloque->tamanioContenido);

	if (msync((void *)archivoMapeado, bloque->tamanioContenido, MS_SYNC) < 0) {
		logErrorAndExit("Error al hacer msync, al setear bloque.");
	}
}

char * getBloque(int posicion){
	char * bloque= malloc(BLOQUE_SIZE);
	memcpy(bloque, archivoMapeado + posicion*BLOQUE_SIZE,BLOQUE_SIZE);
	return bloque;

}

int enviarInfoNodo(int socketFS, TdataNode * dataNode){
	int estado;
	Tbuffer * buffer;

	TpackInfoBloqueDN * infoBloque = malloc(sizeof(TpackInfoBloqueDN));

	infoBloque->head.tipo_de_proceso = DATANODE;
	infoBloque->head.tipo_de_mensaje = INFO_NODO;

	infoBloque->tamanioNombre = strlen(dataNode->nombre_nodo) + 1;
	infoBloque->nombreNodo = malloc(infoBloque->tamanioNombre);
	strcpy(infoBloque->nombreNodo, dataNode->nombre_nodo);
	infoBloque->tamanioIp = strlen(dataNode->ip_nodo) + 1;
	infoBloque->ipNodo = malloc(infoBloque->tamanioIp);
	strcpy(infoBloque->ipNodo, dataNode->ip_nodo);
	infoBloque->tamanioPuerto = strlen(dataNode->puerto_entrada) + 1;
	infoBloque->puertoNodo = malloc(infoBloque->tamanioPuerto);
	strcpy(infoBloque->puertoNodo, dataNode->puerto_worker);//cambio esto
	infoBloque->databinEnMB = dataNode->tamanio_databin_mb;

	buffer = empaquetarInfoNodo(infoBloque);
	 if ((estado = send(socketFS, buffer->buffer , buffer->tamanio, 0)) == -1){
	 		logErrorAndExit("Fallo al enviar a Nodo el bloque a almacenar");
	 	}


	return estado;

}

Tbloque * recvBloque(int socketFS) {
	char* contenidoBloque;
	int estado;
	Tbloque * bloque = malloc(sizeof(Tbloque));
	if ((estado = recv(socketFS, &bloque->nroBloque, sizeof(int), 0)) == -1) {
		logErrorAndExit("Error al recibir el numero de bloque");
	}

	log_info(logInfo,"Recibí el numero de bloque %d\n", bloque->nroBloque);

	if ((estado = recv(socketFS, &bloque->tamanioContenido, sizeof(unsigned long long),
			0)) == -1) {
		logErrorAndExit("Error al recibir el tamaño de bloque");
	}

	bloque->contenido = malloc(bloque->tamanioContenido);
	contenidoBloque = malloc(bloque->tamanioContenido);

	if ((estado = recv(socketFS, contenidoBloque, bloque->tamanioContenido, MSG_WAITALL)) == -1) {
		logErrorAndExit("Error al recibir el contenido del bloque");
	}

	memcpy(bloque->contenido, contenidoBloque, bloque->tamanioContenido);

	free(contenidoBloque);
	return bloque;
}

void enviarBloqueAFS(int nroBloque, int socketFS){
	Tbuffer * buffer;
	Theader * head = malloc(sizeof(Theader));

	head->tipo_de_proceso = DATANODE;
	head->tipo_de_mensaje = OBTENER_BLOQUE_Y_NRO;
	char * bloque = getBloque(nroBloque);

	buffer = empaquetarBytesMasInt(head, bloque, nroBloque);
	if ((send(socketFS, buffer->buffer , buffer->tamanio, 0)) == -1){
		logErrorAndExit("Fallo al enviar a FS el bloque que pidió");
	}

	free(head);
}

void enviarBloque(int nroBloque, unsigned long long int tamanio, int socketFS){
	Tbuffer * buffer;
	Theader * head = malloc(sizeof(Theader));

	head->tipo_de_proceso = DATANODE;
	head->tipo_de_mensaje = OBTENER_BLOQUE;
	log_info(logInfo,"Obtengo el bloque: %d",nroBloque);
	char * bloque = getBloque(nroBloque);

	buffer = empaquetarBloqueConNBytes(head, tamanio, bloque, nroBloque);
	if (send(socketFS, buffer->buffer, buffer->tamanio, 0) <= 0){
		logErrorAndExit("Fallo al enviar a FS el bloque que pidio");
	}
	free(bloque);
	free(head);
	free(buffer->buffer);
	free(buffer);
}
