#ifndef function_h_INCLUDED
#define function_h_INCLUDED

typedef struct Token Token; // tokenizer.h
typedef struct InputSource InputSource; // input_source.h

void get_function_body_single(Token *tk, InputSource *source);
void get_function_body_multi(Token *tk, int func_indent, InputSource *source);

#endif // function_h_INCLUDED
