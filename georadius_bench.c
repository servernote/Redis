#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <hiredis/hiredis.h>
#include <postgresql/libpq-fe.h>

int main(int argc, char **argv) {

	redisContext *rdconn;
	redisReply *rdresp;
	PGconn *pgconn;
	PGresult *pgresp,*pgresp2;
	int i,n;
	int j,q;
	struct timeval tv_fr,tv_to;
	double tm_fr,tm_to;
	char sql[2048];
	char eki[512],lon[256],lat[256];
	double rd_total = 0,pg_total = 0;

	/* 接続 */
	rdconn = redisConnect("127.0.0.1", 6379);
	if (!rdconn) {
		fprintf(stderr, "redisConnect error\n");
		return -1;
	}
	if (rdconn->err) {
		fprintf(stderr, "%s\n", rdconn->errstr);
		redisFree(rdconn);
		return -1;
	}

	pgconn = PQconnectdb( "host=127.0.0.1 port=5432 dbname=ekidb" );
	if(!pgconn || PQstatus( pgconn ) == CONNECTION_BAD ){
		fprintf(stderr, "PQpgconnectdb error\n");
		if(pgconn){
			PQfinish(pgconn);
		}
		redisFree(rdconn);
		return (-1);
	}

	/* ランダムデータはPostgresから取得する */
	sprintf(sql,
/*	"SELECT name, ST_X(geom::geometry) AS longitude, ST_Y(geom::geometry) AS latitude FROM ekipos ORDER BY RANDOM() LIMIT 1000");*/
	"SELECT name, ST_X(geom::geometry) AS longitude, ST_Y(geom::geometry) AS latitude FROM ekipos"); /* 全駅バージョン */
	fprintf(stdout,"%s\n",sql);
	pgresp = PQexec(pgconn, sql);
	if(!pgresp || PQresultStatus( pgresp ) != PGRES_TUPLES_OK ){
		fprintf(stderr, "PQexec error");
		if(pgresp){
			fprintf(stderr," %s",PQresultErrorMessage( pgresp ) );
			PQclear(pgresp);
		}
		fprintf(stderr,"\n");
		PQfinish(pgconn);
		redisFree(rdconn);
		return (-1);
	}

	/* 駅半径50km以内の駅=正方形では100km四方=の駅を抽出するループ */

	for( i = 0,n = PQntuples( pgresp ); i < n; i++ ){
		strcpy(eki,PQgetvalue( pgresp,i,0 ));
		strcpy(lon,PQgetvalue( pgresp,i,1 ));
		strcpy(lat,PQgetvalue( pgresp,i,2 ));

		fprintf(stdout,"%d. %s --------------------\n",i + 1,eki);

		/* redis */
		sprintf(sql,
		"GEORADIUS_RO ekipos %s %s 50000 m WITHDIST COUNT 20 ASC",lon,lat);
		fprintf(stdout,"%s\n",sql);

		q = 0;

		gettimeofday(&tv_fr, NULL);
		rdresp = (redisReply *)redisCommand(rdconn, "GEORADIUS_RO ekipos %s %s 50000 m WITHDIST COUNT 20 ASC",lon,lat);
		gettimeofday(&tv_to, NULL);

		if (rdresp && rdresp->type != REDIS_REPLY_ERROR){
			q = rdresp->elements;
		}

		fprintf(stdout,"hit %d stations on Redis\n",q);

		fprintf(stdout,"unix time before select: %ld.%ld SEC\n",tv_fr.tv_sec,tv_fr.tv_usec);
		fprintf(stdout,"unix time after  select: %ld.%ld SEC\n",tv_to.tv_sec,tv_to.tv_usec);
		
		tm_fr = (((double)tv_fr.tv_sec)*((double)1000000)+((double)tv_fr.tv_usec));
		tm_to = (((double)tv_to.tv_sec)*((double)1000000)+((double)tv_to.tv_usec));

		fprintf(stdout,"diff time: %lf MICROSEC (%lf MSEC)\n",tm_to - tm_fr,(tm_to - tm_fr) / 1000);

		rd_total += (tm_to - tm_fr);

		if(q > 0){
			for(j = 0; j < q; j++){
				fprintf(stdout,"%s %sM\n",rdresp->element[j]->element[0]->str,rdresp->element[j]->element[1]->str);
			}
		}

		if(rdresp){
			freeReplyObject(rdresp);
		}

		/* postgresql */
		sprintf(sql,
		"SELECT name, ST_Distance('SRID=4326;POINT(%s %s)', geom) AS distance"
		" FROM ekipos WHERE ST_DWithin(geom, ST_GeographyFromText('SRID=4326;POINT(%s %s)'), 50000.0)"
		" ORDER BY distance LIMIT 20",lon,lat,lon,lat);
		fprintf(stdout,"%s\n",sql);

		q = 0;

		gettimeofday(&tv_fr, NULL);
		pgresp2 = PQexec(pgconn, sql);
		gettimeofday(&tv_to, NULL);

		if( pgresp2 && PQresultStatus( pgresp2 ) == PGRES_TUPLES_OK ){
			q = PQntuples( pgresp2 );
		}
	
		fprintf(stdout,"hit %d stations on PostgreSQL\n",q);

		fprintf(stdout,"unix time before select: %ld.%ld SEC\n",tv_fr.tv_sec,tv_fr.tv_usec);
		fprintf(stdout,"unix time after  select: %ld.%ld SEC\n",tv_to.tv_sec,tv_to.tv_usec);
		
		tm_fr = (((double)tv_fr.tv_sec)*((double)1000000)+((double)tv_fr.tv_usec));
		tm_to = (((double)tv_to.tv_sec)*((double)1000000)+((double)tv_to.tv_usec));

		fprintf(stdout,"diff time: %lf MICROSEC (%lf MSEC)\n",tm_to - tm_fr,(tm_to - tm_fr) / 1000);

		pg_total += (tm_to - tm_fr);

		if(q > 0){
			for(j = 0; j < q; j++){
				fprintf(stdout,"%s %sM\n",PQgetvalue( pgresp2,j,0 ),PQgetvalue( pgresp2,j,1 ) );
			}
		}

		if(pgresp2){
			PQclear(pgresp2);
		}
	}

	fprintf( stdout,"----------------------------------------\n"
	"Redis GEORADIUS_RO %d call TOTAL = %lf MSEC, AVG = %lf MSEC\n",n, rd_total / 1000,(rd_total / n) / 1000);

	fprintf( stdout,"----------------------------------------\n"
	"PostgreSQL SELECT_ST %d call TOTAL = %lf MSEC, AVG = %lf MSEC\n",n, pg_total / 1000,(pg_total / n) / 1000);

	PQclear(pgresp);
	PQfinish(pgconn);
	redisFree(rdconn);

	return 0;
}
