// Only for parsing function bodies

#pragma once

struct Token; // tokenizer.h

void get_function_body_single(struct Token *tk, FILE *fp);
void get_function_body_multi(struct Token *tk, int func_indent, FILE *fp);
