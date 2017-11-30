all: opt_enc_d opt_enc opt_dec_d opt_dec keygen

opt_enc_d: opt_enc_d.c
	gcc -o $@ $^

opt_enc: opt_enc.c
	gcc -o $@ $^

opt_dec_d: opt_enc_d.c
	gcc -o $@ $^ -D DECRYPT

opt_dec: opt_enc.c
	gcc -o $@ $^ -D DECRYPT


keygen: keygen.c
	gcc -o $@ $^

clean:
	rm -f opt_enc_d opt_enc opt_dec_d opt_dec keygen

.PHONY: clean
