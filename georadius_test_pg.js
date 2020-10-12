var performance = require('perf_hooks').performance;
var { Client } = require('pg');
var client = new Client({
    host: '127.0.0.1',
    port: 5432,
    database: 'ekidb',
});

client.connect();

var query = "SELECT name, ST_X(geom::geometry) AS longitude, ST_Y(geom::geometry) AS latitude," +
" ST_Distance('SRID=4326;POINT(139.394872 35.822019)', geom) AS distance" +
" FROM ekipos WHERE ST_DWithin(geom, ST_GeographyFromText('SRID=4326;POINT(139.394872 35.822019)'), 10000.0)" +
" ORDER BY distance LIMIT 20";

var micro_fr = (Date.now() + performance.now()) * 1000;

client.query(query, (err, res) => {

	var micro_to = (Date.now() + performance.now()) * 1000;

	var data = query + "\n" +
	"関数実行時間 " + (micro_to - micro_fr) + "マイクロ秒\n";
	if(res){
		var i,n = res.rows.length;
		data += "" + n + "個の駅が該当しました。\n";
		for(i = 0; i < n; i++ ){
			data += "" + (i + 1) + " " + res.rows[i].name +
			"(" + res.rows[i].longitude + " " + res.rows[i].latitude + ") " +
			res.rows[i].distance + "M\n";
		}
	}
	else if(err){
		data += err.toString();
	}
	process.stdout.write(data); //出力
    client.end(); //どこかでendしないとプロセスは終わらない
});
