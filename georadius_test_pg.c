#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <postgresql/libpq-fe.h>

int main(int argc, char **argv) {

	PGconn *conn;
	PGresult *res;
	int i,n;
	struct timeval tv_fr,tv_to;
	double tm_fr,tm_to;
	char sql[2048];

	conn = PQconnectdb( "host=127.0.0.1 port=5432 dbname=ekidb" );
	if(!conn || PQstatus( conn ) == CONNECTION_BAD ){
		fprintf(stderr, "PQconnectdb error\n");
		if(conn){
			PQfinish(conn);
		}
		return (-1);
	}

	sprintf(sql,
	"SELECT name, ST_X(geom::geometry) AS longitude, ST_Y(geom::geometry) AS latitude,"
	" ST_Distance('SRID=4326;POINT(139.394872 35.822019)', geom) AS distance"
	" FROM ekipos WHERE ST_DWithin(geom, ST_GeographyFromText('SRID=4326;POINT(139.394872 35.822019)'), 10000.0)"
	" ORDER BY distance LIMIT 20");

	fprintf(stdout,"%s\n",sql);

	gettimeofday(&tv_fr, NULL);

	res = PQexec(conn, sql);

	gettimeofday(&tv_to, NULL);

	tm_fr = (((double)tv_fr.tv_sec)*((double)1000000)+((double)tv_fr.tv_usec));
	tm_to = (((double)tv_to.tv_sec)*((double)1000000)+((double)tv_to.tv_usec));

	fprintf(stdout,"fr=%lf,to=%lf,diff=%lf microsec\n",tm_fr,tm_to,tm_to - tm_fr);
	fprintf(stdout,"関数実行時間 %lfマイクロ秒\n",tm_to - tm_fr);

	n = 0;
	if(res){
		if( PQresultStatus( res ) != PGRES_TUPLES_OK ){
			fprintf(stderr, "PQexec error %s\n",PQresultErrorMessage( res ) );
		}
		else{
			n = PQntuples( res );
		}
	}

	fprintf(stdout, "%d個の駅が該当しました。\n",n);

	for( i = 0; i < n; i++ ){
		fprintf(stdout, "%d %s(%s %s) %sM\n",i + 1,
			PQgetvalue( res,i,0 ),
			PQgetvalue( res,i,1 ),
			PQgetvalue( res,i,2 ),
			PQgetvalue( res,i,3 ) );
	}

	if(res){
		PQclear(res);
	}

	PQfinish(conn);

	return 0;
}
