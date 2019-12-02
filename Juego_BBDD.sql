DROP DATABASE IF EXISTS juego;
CREATE DATABASE juego;

USE juego;

CREATE TABLE jugador (
        usuario text,
        contrasena text,
        id INT NOT NULL,
	PRIMARY KEY(id)
)ENGINE=InnoDB;

INSERT INTO jugador VALUES ('Marc','140100',1);
INSERT INTO jugador VALUES ('Albert','123',2);
INSERT INTO jugador VALUES ('Isma','1456',3);

CREATE TABLE partidas (
	id INT NOT NULL,
	fecha text,
	duracion INT,
        participantes text,
	ganador text,
	puntuacion text,
	PRIMARY KEY (id)
)ENGINE=InnoDB;

INSERT INTO partidas VALUES (1,'5/10/2019',600,'Marc-Albert','Marc','25-10');
INSERT INTO partidas VALUES (2,'6/10/2019',500,'Marc-Albert','Albert','9-12');
INSERT INTO partidas VALUES (3,'12/10/2019',300,'Marc-Isma','Isma','12-9');

CREATE TABLE relacion (
	idpartida INT NOT NULL,
	idjug INT NOT NULL,
	puntuacion INT,
	FOREIGN KEY (idpartida) REFERENCES partidas(id),
        FOREIGN KEY (idjug) REFERENCES jugador(id) 
)ENGINE=InnoDB;

INSERT INTO relacion VALUES (1,1,25);
INSERT INTO relacion VALUES (1,2,10);
INSERT INTO relacion VALUES (2,1,9);
INSERT INTO relacion VALUES (2,2,12);
INSERT INTO relacion VALUES (3,1,12);
INSERT INTO relacion VALUES (3,1,9);



	




