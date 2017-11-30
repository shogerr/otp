all: otp_enc_d otp_enc otp_dec_d otp_dec keygen

otp_enc_d: otp_enc_d.c
	gcc -o $@ $^

otp_enc: otp_enc.c
	gcc -o $@ $^

otp_dec_d: otp_enc_d.c
	gcc -o $@ $^ -D DECRYPT

otp_dec: otp_enc.c
	gcc -o $@ $^ -D DECRYPT


keygen: keygen.c
	gcc -o $@ $^

clean:
	rm -f otp_enc_d otp_enc otp_dec_d otp_dec keygen

.PHONY: clean
