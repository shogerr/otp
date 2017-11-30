all: opt_enc_d opt_enc keygen

opt_enc_d: opt_enc_d.c
	gcc -o $@ $^

opt_enc: opt_enc.c
	gcc -o $@ $^

keygen: keygen.c
	gcc -o $@ $^

clean:
	rm -f opt_enc_d opt_enc

.PHONY: clean
