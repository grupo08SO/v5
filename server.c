// N U E V A   V E R S I O N  D E L   C O D I G O   D E L   S E R V I D O R 
//---------------------------------------------------------------------------------- 
// Se ha reestructurado por motivos de complejidad y resolucin de algunos problemas.

// INCLUDES [YA ESTAN TODOS]
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <mysql.h>
#include <pthread.h>

// DEFINES [SE PUEDEN INCLUIR MAS]
#define TRUE 1
#define FALSE 0
#define MAX_NUM_USUARIOS 100
#define PORT 9103
#define MAX_FORMAT_USER 20
#define MAX_FORMAT_INPUT 80
#define MAX_CHAR 512

//mysql defines ---> para cambiar de host en localhost a shiva, solo hay que cambiar
//el "localhost" por "tu dirección de shiva"
#define HOST "localhost"
#define USR "root"
#define PSWD "mysql"

// ESTRUCTURAS PARA EL FORMATO DE NOMBRES, ETC
/*
con esto podemos declarar variables del tipo X para que sea mas facil de entender
a la hora de cambiar est o es mas escalable.
*/

typedef char nombre[MAX_FORMAT_USER]; //nuevo formato para los usuarios
typedef char contrasena[MAX_FORMAT_USER]; //nuevo formato para la contrasena
typedef char input[MAX_FORMAT_INPUT];
typedef char buffer[MAX_CHAR];
typedef char query[MAX_CHAR];

// LISTAS Y CLASES
typedef struct{
	nombre username;
	int socket;
}Usuario;

typedef struct{
	Usuario usuario[MAX_NUM_USUARIOS];
	int num;
}TUsuarios;

typedef struct{
	Usuario x;
	Usuario y;
	int id;
}Partida;

typedef struct{
	Partida partida[100];
	int numero;
}TablaPartidas;
// VARIABLES GLOBALES
//mysql
MYSQL *mysql_conn;
MYSQL_RES *res;
MYSQL_ROW *row;

//usuario
TUsuarios usuarios[MAX_NUM_USUARIOS];

//socket & server
struct sockaddr_in serv_adr;
int serverSocket;
int sock_atendedidos[MAX_NUM_USUARIOS];

//threads
pthread_t thread[MAX_NUM_USUARIOS];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

//Tabla Partidas
TablaPartidas games;

// FUNCIONES DE LISTA DE CONNECTADOS
void GetConnected(buffer notif){
	int i;
	buffer substitute;
	strcpy(notif, "6/");
	
	for(i = 0; i < usuarios->num; i++){
		strcpy(substitute, notif);
		sprintf(notif, "%s%s ", substitute, usuarios->usuario[i].username);
	}
}


int DarPosicionSocket(int socket){
	int encontrado = FALSE, i = 0;
	while((!encontrado)&&(i < usuarios->num)){
		if(usuarios->usuario[i].socket == socket)
			encontrado = TRUE;
		else
			i++;
	}
	if(encontrado)
		return i;
	else
		return -1;
}

int AddToList(nombre user, int socket){
	if(usuarios->num < MAX_NUM_USUARIOS){
		strcpy(usuarios->usuario[usuarios->num].username,user);
		usuarios->usuario[usuarios->num].socket = socket;
		
		usuarios->num++;
		
		return TRUE;
	}
	else{
		printf("Lista de usuarios completa.\n");
		return FALSE;
	}
}

int RemoveFromList(int socket){
	int posicionLista = DarPosicionSocket(socket), i;
	if(posicionLista >= 0){
		for(i = posicionLista; i < usuarios->num; i++){
			strcpy(usuarios->usuario[i].username, usuarios->usuario[i + 1].username);
			usuarios->usuario[i].socket = usuarios->usuario[i + 1].socket;
		}
		usuarios->num--;
	}
	else
	   return FALSE;
}

int DevolverSocket(nombre user){
	int encontrado = FALSE, i = 0;
	while((!encontrado)&&(i < usuarios->num)){
		if(strcmp(usuarios->usuario[i].username, user))
			encontrado = TRUE;
		else
			i++;
	}
	if(encontrado)
		return usuarios->usuario[i].socket;
	else
		return -1;
}

void DevolverNombre(int socket, nombre user){
	int encontrado = FALSE, i = 0;
	while((!encontrado)&&(i < usuarios->num)){
		if(usuarios->usuario[i].socket == socket){
			strcpy(user, usuarios->usuario[i].username);
			encontrado = TRUE;
		}
		else
		   i++;
	}
}

int AnadirPartida(Partida p){
	int encontrado=0;
	int i=0;
	while ((i<100)&&(encontrado==0))
	{
		if (games.partida[i].id==-1)
			encontrado=1;
		else i++;
	}
	if (encontrado==1)
	{
		games.partida[i]=p;
		games.numero++;
		p.id=i;
		return TRUE;
	}
	else return FALSE;
};

int DevolverRival(nombre jugador, nombre rival){
	int encontrado=0;
	int i=0;
	while ((i<100)&&(encontrado=0))
	{
		if (strcmp(games.partida[i].x.username,jugador)==0)
	    {
			strcpy(rival,games.partida[i].y.username);
			return TRUE;
	    }
		else if (strcmp(games.partida[i].y.username,jugador)==0)
		{
			strcpy(rival,games.partida[i].x.username);
			return TRUE;
		}
		else return FALSE;
    }
};

// INICIADORES
/*
funcion para el inicio y config del servidor
aqui solo configuramos la IP y el puerto en uso
¡¡¡PARA CAMBIAR DE PUERTO CAMBIAMOS DEL DEFINE EL PORT!!!
*/
void InitServer(){
	//CONFIGURACION DEL SERVIDOR
	printf("[Iniciando Servidor...]\n");
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	printf("Habilitamos serverSocket.\n");
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(PORT);
	printf("Configuración del server lista, escuchando en puerto: %d\n", PORT);
}


/*
Bind con el puerto:
*/
void InitBind(){
	//CONFIGURACION DEL BIND
	printf("[Iniciando Bind...]\n");
	if (bind(serverSocket, (struct sockaddr *) &serv_adr, sizeof(serv_adr)) < 0){
		printf ("Error al bind\n");
		exit(1);
	}
	else{
		printf("Bind creado correctamente.\n");
		
		//Comprobamos que el servidor este escuchando
		if (listen(serverSocket, 2) < 0){
			printf("Error en el Listen");
			exit(1);
		}
		else
			printf("serverSocket funcionando correctamente.\n[Todo OK.]\n");
	}
}



/*
Iniciamos la base de datos:
*/
void InitBBDD(){
	//CONFIGURAMOS LA CONEXION BASEDATOS CON SERVIDOR C
	int err;
	mysql_conn = mysql_init(NULL);
	if (mysql_conn==NULL) 
	{
		printf ("Error al crear la conexion: %u %s\n", 
				mysql_errno(mysql_conn), mysql_error(mysql_conn));
		exit (1);
	}
	//inicializar la conexion, indicando nuestras claves de acceso
	// al servidor de bases de datos (user,pass)
	mysql_conn = mysql_real_connect (mysql_conn, HOST, USR, PSWD, NULL, 0, NULL, 0);
	if (mysql_conn==NULL)
	{
		printf ("Error al inicializar la conexion: %u %s\n", 
				mysql_errno(mysql_conn), mysql_error(mysql_conn));
		exit (1);
	}
	
	//indicamos la base de datos con la que queremos trabajar 
	err=mysql_query(mysql_conn, "use juego;");
	if (err!=0)
	{
		printf ("Error al conectar con la base de datos %u %s\n", 
				mysql_errno(mysql_conn), mysql_error(mysql_conn));
		exit (1);
	}
}

void InitTablaPartidas(){
	for (int i=0; i<100; i++)
	{
		games.partida[i].id=-1;
	}
	
}

// CERRAR BBDD
void CerrarBBDD(){
	mysql_close(mysql_conn);
}



// CONSULTAS EN LA BBDD
void ConsultaBBDD(query consulta){
	int err;
	err = mysql_query(mysql_conn, consulta);
	if(err){
		printf ("Error al crear la conexion: %u %s\n", 
				mysql_errno(mysql_conn), mysql_error(mysql_conn));
		exit (1);
	}
	else{
		res = mysql_store_result(mysql_conn); //Esto se asigna a la variable global MYSQL_RES *res
	}
}


int EncontrarJugador(nombre usuario){
	//Queremos encontrar un jugador en la tabla de jugadores.
	query consulta;
	
	sprintf(consulta, "SELECT id FROM jugador WHERE usuario = '%s';", usuario);
	
	ConsultaBBDD(consulta);
	row = mysql_fetch_row(res);
	
	if (row == NULL)
		return FALSE;
	else
		return TRUE;
}


int ExisteID(int ID){
	query consulta;
	sprintf(consulta, "SELECT usuario FROM jugador WHERE id = %d;", ID);
	
	ConsultaBBDD(consulta);
	row = mysql_fetch_row(res);
	
	if(row == NULL)
		return FALSE; //Si no existe la ID, devolvemos 0
	else
		return TRUE; //Si existe, devolvemos 1
}


int LogIN(input in, buffer output, int socket){
	nombre user1;
	contrasena contra1;
	query consulta;
	
	//obtenemos la informacion
	sscanf(in, "%s %s", user1, contra1);
	
	//Mirem si existeix tal usuari:
	pthread_mutex_lock(&lock);
	if (EncontrarJugador(user1)){
		sprintf(consulta, "SELECT contrasena FROM jugador WHERE usuario = '%s';", user1);
		
		//Obtenemos el resultado de la consulta.
		ConsultaBBDD(consulta); //Ya se ha asignado el valor de la consulta al res.
		row = mysql_fetch_row(res);
		
		if(row != NULL){
			if(!strcmp(row[0], contra1)){
				pthread_mutex_unlock(&lock);
				if(AddToList(user1, socket)){
					strcpy(output, "1/1");
					return TRUE;
				}
				else{
					strcpy(output, "1/0");
					return FALSE;
				}
			}
			else{
				strcpy(output, "1/0");
				return FALSE;
			}
		}
		else{
			pthread_mutex_unlock(&lock);
			strcpy(output, "1/1");
			return FALSE;
		}
	}
	else{
		pthread_mutex_unlock(&lock);
		strcpy(output, "1/0"); //No existe, retornamos 0.
		return FALSE;
	}
}


int SignUP(input in, buffer output, int socket){
	nombre user;
	contrasena contra;
	query consulta;
	int ID, existe = TRUE;
	//Obtener la informaciÃ³n del input.
	sscanf(in,"%s %s", user, contra);
	
	pthread_mutex_lock(&lock);
	ID = rand();
	if(ExisteID(ID)){ //Si existe la ID, hay que cambiarla
		while(existe){
			ID = rand();
			existe = ExisteID(ID);
		}
		existe = TRUE; //Como la ID random ahora no existe podemos decir que la ID nueva esta libre con lo cual puede existir.
	}
	if(existe){ //Si la ID está libre, significa que puede existir
		if(!EncontrarJugador(user)){
			sprintf(consulta, "INSERT INTO jugador (id, usuario, contrasena) VALUES(%d ,'%s', '%s')", ID ,user, contra);
			ConsultaBBDD(consulta); //Registrado
			
			if(EncontrarJugador(user)){
				pthread_mutex_unlock(&lock);
				if(AddToList(user, socket)){
					strcpy(output, "2/1");
					return TRUE;
				}
				else{
					strcpy(output, "2/0");
					return FALSE;
				}
			}
			else{
				pthread_mutex_unlock(&lock);
				strcpy(output, "2/0");
				return FALSE;
			}
		}
	}
}

int PuntosTotales(input in, buffer output){
	nombre usuario;
	query consulta;
	
	//Queremos obtener el usuario del cual recoger la puntuaciÃ³n total:
	sscanf(in, "%s", usuario);
	
	pthread_mutex_lock(&lock);
	if(EncontrarJugador(usuario)){
		//Queremos recoger el valor de la consulta:
		sprintf(consulta, "SELECT SUM(relacion.puntuacion) FROM jugador, relacion, partidas WHERE relacion.idjug = jugador.id AND jugador.usuario ='%s';", usuario);
		ConsultaBBDD(consulta);
		
		row = mysql_fetch_row(res);
		if(row[0] != NULL){
			int result = atoi(row[0]);
			
			pthread_mutex_unlock(&lock);
			sprintf(output, "3/%d", result);
			return TRUE;
		}
		else{
			pthread_mutex_unlock(&lock);
			sprintf(output, "3/0");
			return FALSE;
		}
	}
	else{
		pthread_mutex_unlock(&lock);
		sprintf(output, "3/0");
		return FALSE;
	}	
}

int TiempoTotalJugado(input in, buffer output){
	nombre usuario;
	query consulta;
	
	sscanf(in, "%s", usuario);
	
	pthread_mutex_lock(&lock);
	if(EncontrarJugador(usuario)){
		sprintf(consulta, "SELECT SUM(partidas.duracion) FROM partidas, jugador, relacion WHERE partidas.id = relacion.idpartida AND relacion.idjug = jugador.id AND jugador.usuario = '%s';", usuario);
		ConsultaBBDD(consulta);
		
		row = mysql_fetch_row(res);
		
		if(row[0] == NULL){
			pthread_mutex_unlock(&lock);
			strcpy(output, "4/0");
			return FALSE;
		}
		else{
			int result = atoi(row[0]);
			pthread_mutex_unlock(&lock);
			sprintf(output, "4/%d", result);
			return TRUE;
		}
	}
	else{
		pthread_mutex_unlock(&lock);
		strcpy(output, "4/0");
		return FALSE;
	}
}

int PartidasGanadasVS(input in, buffer output){
	nombre usuario;
	query consulta;
	
	sscanf(in, "%s", usuario);
	
	pthread_mutex_lock(&lock);
	if(EncontrarJugador(usuario)){
		sprintf(consulta, "SELECT COUNT(ganador) FROM partidas WHERE ganador = '%s';", usuario);
		ConsultaBBDD(consulta);
		
		row = mysql_fetch_row(res);
		
		if(row[0] == NULL){
			pthread_mutex_unlock(&lock);
			strcpy(output, "5/0");
			return FALSE;
		}
		else{
			int result = atoi(row[0]);
			pthread_mutex_unlock(&lock);
			sprintf(output, "5/%d", result);
			return TRUE;
		}
	}
	else{
		pthread_mutex_unlock(&lock);
		strcpy(output, "5/0");
		return FALSE;
	}
}


// TRHEADS Y EJECUCION DE CODIGO
int DarCodigo(buffer buff,input in){
	char *p =strtok(buff,"/");
	int codigo = atoi(p);
	if(codigo != 0){ //Si el codi es 0 no fa falta seguir troçejant
		p = strtok(NULL, "/");
		strcpy(in,p);
	}
	
	return codigo;
}

int EnviarInvitacion(input in, buffer output, int socket){
	//Separamos el mensaje 
	nombre userInvitado;
	strcpy(userInvitado, in);
	
	//SOCKET DEL INVITADO
	int socketInvitado = DevolverSocket(userInvitado);
	//DevolverSocket devuelve -1 si no existe tal socket:
	if(socketInvitado > 0){
		buffer mensaje;
		nombre userHost; //userHost es el LIDER DE LA PARTIDA
		DevolverNombre(socket, userHost); //Recogemos el nombre del LIDER
		sprintf(mensaje, "7/%s", userHost); //Al mensaje que enviar le decimos el nombre de usuario del LIDER
		write(socketInvitado, mensaje, strlen(mensaje)); //Enviamos al invitado.
		strcpy(output, "10/1"); //Enviamos si se ha podido enviar con exito.
		return TRUE;
	}
	else{
		strcpy(output, "10/0"); //Sin exito.
		return FALSE;
	}
}

int EnviarRechazo(input in, buffer output, int socket){
	/*
	Para el rechazo, el invitado envia:
	"9/userHost"
	
	Al Host de la partida se le manda:
	"9/userInvitado"
	
	De esta manera solo hay que recoger el socket del lider a traves de la funcion dame socket.
	*/
	nombre userHost;
	nombre userInvitado;

	//userHost
	sscanf(in, "%S", userHost);
	
	//userInvitado
	DevolverNombre(socket, userInvitado);
	
	//ScoketHost
	int socketHost = DevolverSocket(userHost);
	
	//Finalmente enviamos las peticiones
	if(socketHost > 0){
		buffer mensaje;
		
		sprintf(mensaje, "9/%s", userInvitado);
		write(socketHost, mensaje, strlen(mensaje));
		
		strcpy(output, "11/1");
		return TRUE;
	}
	else{
		strcpy(output, "11/0");
		return FALSE;
	}
}

int EnviarAceptar(input in, buffer output, int socket){
	/*
	Para Aceptar, el invitado envia:
	"8/userHost"
	
	Al Host de la partida se le envia:
	"8/userInvitado"
	
	De esta manera solo hay que recoger el socket del lider a traves de la funcion dame socket.
	*/
	nombre userHost;
	nombre userInvitado;
	
	//userHost
	sscanf(in, "%S", userHost);
	
	//userInvitado
	DevolverNombre(socket, userInvitado);
	
	//ScoketHost
	int socketHost = DevolverSocket(userHost);
	
	//Finalmente enviamos las peticiones
	if(socketHost > 0){
		buffer mensaje;
		
		sprintf(mensaje, "8/%s", userInvitado);
		write(socketHost, mensaje, strlen(mensaje));
		
		strcpy(output, "12/1");
		
		return TRUE;
	}
	else{
		strcpy(output, "12/0");
		return FALSE;
	}
}

int EnviarMensaje(input in, buffer output, int socket){
	/*
	Para Enviar mensaje, el emisor envia:
	"13/Receptor+mensaje"
	
	Al Receptor se le envia:
	"13/Emisor+mensaje"
	
	De esta manera solo hay que recoger el socket del receptor a traves de la funcion dame socket.
	*/
	nombre Receptor;
	nombre Emisor;
	buffer mensaje;
	
	//userHost
	sscanf(in, "%s %s", Receptor,mensaje);
	
	//Emisor
	DevolverNombre(socket, Emisor);
	
	//ScoketReceptor
	int socketReceptor = DevolverSocket(Receptor);
	
	//Finalmente enviamos las peticiones
	if(socketReceptor>= 0){
		buffer msg;
		sprintf(msg, "13/%s %s",Emisor, mensaje);
		write(socketReceptor, msg, strlen(msg));
		printf("Enviamos a %s: %s\n",Receptor,msg);
		strcpy(output, "14/1");
		return TRUE;
	}
	else{
		strcpy(output, "14/0");
		return FALSE;
	}
}

void EjecutarCodigo(buffer buff, int socket, buffer output, int *start){
	input in;
	buffer notif;
	int codigo;
	codigo = DarCodigo(buff, in);
	
	if (codigo == 1){
		if(LogIN(in, output, socket)){
			printf("Socket %d se ha connectado.\n", socket);
			
			GetConnected(notif);
			
			int i;
			for(i = 0; i < usuarios->num; i++)
				write(usuarios->usuario[i].socket, notif, strlen(notif));
			printf("Enviamos:%s\n",notif);
		}
		else
			printf("Hubo algún problema.\n");
	}
	else if (codigo == 2){
		if(SignUP(in, output, socket)){
			printf("Socket %d se ha registrado y conectado.\n", socket);
			
			GetConnected(notif);
			
			int i;
			for(i = 0; i < usuarios->num; i++)
				write(usuarios->usuario[i].socket, notif, strlen(notif));
		
		}
		else
			printf("Hubo algún problema.\n");
	}
	else if (codigo == 3){
		if(PuntosTotales(in, output)>0)
			printf("Success.\n");
		else
			printf("Hubo algún problema.\n");
	}
	else if (codigo == 4){
		if(TiempoTotalJugado(in, output)>0)
			printf("Success.\n");
		else
			printf("Hubo algún problema.\n");
	}
	else if (codigo == 5){
		if(PartidasGanadasVS(in, output) != 0)
			printf("Success.\n");
		else
			printf("Hubo algún problema.\n");
	}
	else if (codigo == 7){
		if(EnviarInvitacion(in, output, socket))
			printf("Success.\n");
		else
			printf("Hubo algun problema.\n");
	}
	else if (codigo == 8){
		if(EnviarAceptar(in, output, socket))
			printf("Success.\n");
		else
			printf("Hubo algun problema.\n");
	}
	else if (codigo == 9){
		if(EnviarRechazo(in, output, socket))
			printf("Success.\n");
		else
			printf("Hubo algun problema.\n");
	}
	else if (codigo == 13){
		if(EnviarMensaje(in, output, socket))
		printf("Success.\n");
		else 
			printf("Hubo algun problema.\n");
	}
	else if(codigo == 0){
		*start = FALSE;
		RemoveFromList(socket);
		
		GetConnected(notif);
		
		int i;
		for(i = 0; i < usuarios->num; i++)
			write(usuarios->usuario[i].socket, notif, strlen(notif));
		
		printf("Desconexión Socket %d\n", socket);
	}
}

void *ThreadExecute (void *socket){
	buffer buff;
	buffer output;
	int sock_att, start = TRUE, ret;
	int *s;
	s = (int *)socket;
	sock_att = *s;
	while (start){
		ret = read(sock_att, buff, sizeof(buff));
		buff[ret] = '\0';
		
		printf("Recibida orden de Socket %d.\n", sock_att);
		printf("Pide: \n");
		puts(buff);
		
		EjecutarCodigo(buff ,sock_att, output, &start);
		
		if(start){
			printf("Enviamos: %s\n", output);
			write(sock_att, output, strlen(output));
		}
	}
}


int main(int argc, char *argv[]) {
	
	InitServer();
	InitBind();
	InitBBDD();
	InitTablaPartidas();
	
	int attendSocket, i = 0;
	// Atender las peticiones
	for( ; ; ){
		printf ("Escuchando\n");
		attendSocket= accept(serverSocket, NULL, NULL);
		printf ("He recibido conexion\n");
		//attendSockeet es el socket que usaremos para un cliente
		
		sock_atendedidos[i] = attendSocket;
		
		pthread_create(&thread[i], NULL, ThreadExecute, &sock_atendedidos[i]); //Thread_que_toque_usar, NULL, Nombre_de_la_funcion_del_thread, socket_del cliente
		i++;
	}
	
	CerrarBBDD();
}

