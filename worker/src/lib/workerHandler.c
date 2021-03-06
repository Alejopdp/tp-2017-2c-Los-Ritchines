/*
 * workerHandler.c
 *
 *  Created on: 2/11/2017
 *      Author: utnso
 */

#include "funcionesWK.h"

extern t_list * listaApareos;

int manejarConexionWorker(Theader *head, int client_sock){
	//printf("En manejar conexion worker");
	char * buffer;

	//todo revisar
	//int p=0;
	//int k=0;
	int stat;
//	Theader headRcv = {.tipo_de_proceso = WORKER, .tipo_de_mensaje = 0};


	if(head->tipo_de_mensaje==GIVE_TMPREDUCCIONLOCAL){
		puts("Se conecto nodo encargado para hacer el apareo global");
		puts("realizando apareo..");
		log_info(logInfo,"Nos llega el path del archivo temporal que precisa");
		TpackBytes *pathArchivoTemporal;

		if ((buffer = recvGeneric(client_sock)) == NULL){
			puts("Fallo recepcion del path del archivo temporal");
			return FALLO_RECV;
		}

		if ((pathArchivoTemporal =  deserializeBytes(buffer)) == NULL){
			puts("Fallo deserializacion de Bytes del path arch a reducir");
			return FALLO_GRAL;
		}
		free(buffer);

		log_info(logInfo,"Path archivo que vamos a enviarle: %s",pathArchivoTemporal->bytes);
		FILE * fdTempFilePropio;
		fdTempFilePropio = fopen((pathArchivoTemporal->bytes),"r");

		Tapareo *apareo = malloc(sizeof(Tapareo));
		apareo->fdWorker=client_sock;
		apareo->fdTemporal=fdTempFilePropio;
		list_add(listaApareos,apareo);
		free(pathArchivoTemporal->bytes);
		free(pathArchivoTemporal);

		puts("comienza el apareo global");
		//printf("fdw %d fdtf %d \n",apareo->fdWorker,apareo->fdWorker);

	}else if(head->tipo_de_mensaje==GIVE_NEXTLINE){
		//puts("me pide nextline");
		char * lineaAux = malloc(MAXSIZELINEA);
		int packSize=0;
		Theader headEnvio;
		headEnvio.tipo_de_proceso=WORKER;
		headEnvio.tipo_de_mensaje=TAKE_NEXTLINE;
		if(fgets(lineaAux, 1024*1024,getFDTemporal(client_sock)) !=NULL){
			//log_info(logInfo,"Envio: %s\n",lineaAux);
			//printf("Envio: %s\n",lineaAux);
			buffer=serializeBytes(headEnvio,lineaAux,strlen(lineaAux)+1,&packSize);
			if ((stat = send(client_sock, buffer, packSize, 0)) == -1){
				puts("no se pudo enviar path del archivo temporal que necesitamos. ");
				return FALLO_SEND ;
			}
			//log_info(logInfo,"41");
			free(buffer);
			//log_info(logInfo,"42");
			free(lineaAux);
			//log_info(logInfo,"43");
		}else{
			head->tipo_de_mensaje=EOF_TEMPORAL;
			head->tipo_de_proceso=WORKER;
			puts("Fin Conexion con encargado");
			enviarHeader(client_sock,head);
			fclose(getFDTemporal(client_sock));
			removerFDWorker(client_sock);
			//puts("antes free");
			free(lineaAux);
			//puts("pase free");
		}
	}

	////////////////////////////////////////////////

	/*
		while ((stat=recv(client_sock, &headRcv, HEAD_SIZE, 0)) > 0) {

			switch (headRcv.tipo_de_mensaje) {

			case(GIVE_NEXTLINE):
				//log_info(logInfo,"give next");
						p++;
						if(p==3000){
							p=0;
							k++;
							printf("realizando el apareo global (%d)\n",k);
						}
						headEnvio.tipo_de_proceso=WORKER;
						headEnvio.tipo_de_mensaje=TAKE_NEXTLINE;
						if(fgets(lineaAux, 1024*1024,fdTempFilePropio) !=NULL){
							//log_info(logInfo,"Envio: %s\n",lineaAux);
							//printf("Envio: %s\n",lineaAux);
							buffer=serializeBytes(headEnvio,lineaAux,strlen(lineaAux)+1,&packSize);
							if ((stat = send(client_sock, buffer, packSize, 0)) == -1){
								puts("no se pudo enviar path del archivo temporal que necesitamos. ");
								return FALLO_SEND ;
							}
							//log_info(logInfo,"41");
							free(buffer);
							//log_info(logInfo,"42");
							//free(lineaAux);
							//log_info(logInfo,"43");
						}else{
							head->tipo_de_mensaje=EOF_TEMPORAL;
							head->tipo_de_proceso=WORKER;
							//puts("le mando eof");
							enviarHeader(client_sock,head);
							fclose(fdTempFilePropio);
						}

			break;
			default:
				printf("mensaje no reconocido proceso: %d msj: %d\n",head->tipo_de_proceso, head->tipo_de_mensaje);
				break;
			}
		}
	}*/
	//puts("fin conexion con worker encargado");
	//log_info(logInfo,"free linea axu");
	//free(lineaAux);
	//log_info(logInfo,"pase free linea aux wh");

	return 0;
}

FILE * getFDTemporal(int fdWorker){
	int i;
	for(i=0;i<list_size(listaApareos);i++){
		Tapareo *aux = list_get(listaApareos,i);
		if(aux->fdWorker==fdWorker){
			return aux->fdTemporal;
		}
	}
	puts("error fd temporal");
	return NULL;
}
void removerFDWorker(int fdWorker){
	int i;
	for(i=0;i<list_size(listaApareos);i++){
		Tapareo *aux = list_get(listaApareos,i);
		if(aux->fdWorker==fdWorker){
			list_remove(listaApareos,i);
			return;
		}
	}

	puts("error remover");
	return;
}
