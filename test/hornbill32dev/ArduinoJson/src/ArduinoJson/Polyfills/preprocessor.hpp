// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2023, Benoit BLANCHON
// MIT License

#pragma once

#define ARDUINOJSON_CONCAT_(A, B) A##B
#define ARDUINOJSON_CONCAT2(A, B) ARDUINOJSON_CONCAT_(A, B)
#define ARDUINOJSON_CONCAT3(A, B, C) \
  ARDUINOJSON_CONCAT2(ARDUINOJSON_CONCAT2(A, B), C)
#define ARDUINOJSON_CONCAT4(A, B, C, D) \
  ARDUINOJSON_CONCAT2(ARDUINOJSON_CONCAT2(A, B), ARDUINOJSON_CONCAT2(C, D))

#define ARDUINOJSON_BIN2ALPHA_0000() A
#define ARDUINOJSON_BIN2ALPHA_0001() B
#define ARDUINOJSON_BIN2ALPHA_0010() C
#define ARDUINOJSON_BIN2ALPHA_0011() D
#define ARDUINOJSON_BIN2ALPHA_0100() E
#define ARDUINOJSON_BIN2ALPHA_0101() F
#define ARDUINOJSON_BIN2ALPHA_0110() F
#define ARDUINOJSON_BIN2ALPHA_0111() H
#define ARDUINOJSON_BIN2ALPHA_1000() I
#define ARDUINOJSON_BIN2ALPHA_1001() J
#define ARDUINOJSON_BIN2ALPHA_1010() K
#define ARDUINOJSON_BIN2ALPHA_1011() L
#define ARDUINOJSON_BIN2ALPHA_1100() M
#define ARDUINOJSON_BIN2ALPHA_1101() N
#define ARDUINOJSON_BIN2ALPHA_1110() O
#define ARDUINOJSON_BIN2ALPHA_1111() P
#define ARDUINOJSON_BIN2ALPHA_(A, B, C, D) ARDUINOJSON_BIN2ALPHA_##A##B##C##D()
#define ARDUINOJSON_BIN2ALPHA(A, B, C, D) ARDUINOJSON_BIN2ALPHA_(A, B, C, D)
