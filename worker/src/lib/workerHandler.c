/*
 * workerHandler.c
 *
 *  Created on: 2/11/2017
 *      Author: utnso
 */

#include "funcionesWK.h"





int manejarConexionWorker(Theader *head, int client_sock){

	char * buffer;
	TpackBytes *pathArchivoTemporal;
	char * lineaAux = malloc(MAXSIZELINEA);//todo revisar


	if(head->tipo_de_mensaje==GIVE_TMPREDUCCIONLOCAL){

		//puts("Nos llega el path del archivo temporal que precisa");


		if ((buffer = recvGeneric(client_sock)) == NULL){
			puts("Fallo recepcion del path del archivo temporal");
			return FALLO_RECV;
		}

		if ((pathArchivoTemporal =  deserializeBytes(buffer)) == NULL){
			puts("Fallo deserializacion de Bytes del path arch a reducir");
			return FALLO_GRAL;
		}

		//printf("Path archivo que vamos a enviarle: %s\n",pathArchivoTemporal->bytes);
		FILE * fdTempFilePropio;
		fdTempFilePropio = fopen((pathArchivoTemporal->bytes),"r");

		char * buffer;
		int packSize=0;
		Theader headEnvio;

		int stat;
		Theader headRcv = {.tipo_de_proceso = WORKER, .tipo_de_mensaje = 0};
		while ((stat=recv(client_sock, &headRcv, HEAD_SIZE, 0)) > 0) {

			switch (headRcv.tipo_de_mensaje) {

			case(GIVE_NEXTLINE):
					//puts("give next");
						headEnvio.tipo_de_proceso=WORKER;
						headEnvio.tipo_de_mensaje=TAKE_NEXTLINE;
						if(fscanf (fdTempFilePropio, "%s", lineaAux)!=-1){
							//printf("Envio: %s\n",lineaAux);
							buffer=serializeBytes(headEnvio,lineaAux,strlen(lineaAux)+1,&packSize);
							if ((stat = send(client_sock, buffer, packSize, 0)) == -1){
								puts("no se pudo enviar path del archivo temporal que necesitamos. ");
								return FALLO_SEND ;
							}
						}else{
							head->tipo_de_mensaje=EOF_TEMPORAL;
							head->tipo_de_proceso=WORKER;
							enviarHeader(client_sock,head);
						}

			break;
			default:
				printf("mensaje no reconocido proceso: %d msj: %d\n",head->tipo_de_proceso, head->tipo_de_mensaje);
				break;
			}
		}
	}
	puts("fin conexion con worker encargado");
	//free(lineaAux);
	return 0;
}
