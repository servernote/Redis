var performance = require('perf_hooks').performance;
var redis = require('redis');
var client = redis.createClient();

var lat = 35.822019;
var lng = 139.394872;
var rad = 10000;
var cnt = 20;

var argArray = ['ekipos', lng, lat, rad, 'm', 'WITHDIST', 'ASC'];
argArray.push('COUNT', cnt); //COUNT 20 などとするにはこうpushするのがポイント

var micro_fr = (Date.now() + performance.now()) * 1000;

client.send_command('GEORADIUS_RO', argArray, function (err, reply) {

	var micro_to = (Date.now() + performance.now()) * 1000;

	var data = "GEORADIUS_RO ekipos " + lng + " " + lat + " " + rad + " " + "m" + " " + "WITHDIST" + " " + "COUNT" + " " + cnt + " " + "ASC" + "\n" +
	"関数実行時間 " + (micro_to - micro_fr) + "マイクロ秒\n";
	if(reply){
		var i,n = reply.length;
		data += "" + n + "個の駅が該当しました。\n";
		for(i = 0; i < n; i++ ){
			data += reply[i][1] + "M " + reply[i][0] + "\n";
		}
	}
	else if(err){
		data += err.toString();
	}
	process.stdout.write(data); //出力
	client.quit(); //どこかでquitしないとプロセスは終わらない
});
