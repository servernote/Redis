<?php date_default_timezone_set('Asia/Tokyo');

$file = new SplFileObject('station20151215free.txt', 'r');
$file->setFlags(SplFileObject::READ_CSV | SplFileObject::SKIP_EMPTY | SplFileObject::READ_AHEAD);
$file->setCsvControl("\t");

foreach ($file as $line)
{
    $fields[] = $line;
}

$n = count($fields);
$i = 9; // fields[9]からが実データ

for( ; $i < $n; $i++ ){
    // fields[i][10]=経度,[11]=緯度,[3]=駅名,[7]=都道府県番号
    // 駅名は同一駅名があるため、駅名-都道府県番号 という感じで登録する。
    $pref_no = $fields[$i][7];
    if(strlen($pref_no) <= 1){
      $pref_no = "0" . $pref_no;
    }
    echo "INSERT INTO ekipos (name, geom) VALUES (" .
    "E'" . $fields[$i][3] . "-" . $pref_no . "', " .
    //駅名-都道府県番号でなく行番号-駅名で登録する場合
    //"E'" . ($i - 9 + 1) . "-" . $fields[$i][3] . "', " .
    "ST_GeographyFromText('SRID=4326;POINT(" .
    $fields[$i][10] . " " .
    $fields[$i][11] . ")'" . "));\n";
}
