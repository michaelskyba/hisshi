#ifndef function_h_INCLUDED
#define function_h_INCLUDED

struct Token; // tokenizer.h
struct InputSource; // input_source.h

void get_function_body_single(struct Token *tk, struct InputSource *source);
void get_function_body_multi(struct Token *tk, int func_indent, struct InputSource *source);

#endif // function_h_INCLUDED
