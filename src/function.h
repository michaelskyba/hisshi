#pragma once

struct InputSource; // input_source.h
struct Token; // tokenizer.h

void get_function_body_single(struct Token *tk, struct InputSource *source);
void get_function_body_multi(struct Token *tk, int func_indent, struct InputSource *source);
