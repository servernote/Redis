#include <stdio.h>
#include <hiredis/hiredis.h>
#include <stdlib.h>

extern int main(int argc, char **argv) {
  redisContext *con;
  redisReply *rep;
  int i;

  /* 接続 */
  con = redisConnect("127.0.0.1", 6379);
  if (!con) {
    fprintf(stderr, "redisConnect error\n");
    return -1;
  }
  if (con->err) {
    fprintf(stderr, "%s\n", con->errstr);
    redisFree(con);
    return -1;
  }

  /* データ表示 */
  for (i = 1; i <= 3; i++) {
    rep = (redisReply *)redisCommand(con, "GET KEY-%d", i);
    if (!rep) {
      fprintf(stderr, "redisReply error\n");
      redisFree(con);
      return -1;
    }
    if (rep->type != REDIS_REPLY_ERROR) {
      printf("KEY-%d = %s\n", i, rep->str);
    }
    freeReplyObject(rep);
  }

  /* 終了 */
  redisFree(con);
  return 0;
}
