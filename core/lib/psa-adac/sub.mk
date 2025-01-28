global-incdirs-y += psa-adac/core/include
srcs-y += psa-adac/core/src/adac_certificate.c
srcs-y += psa-adac/core/src/adac_crypto.c
srcs-y += psa-adac/core/src/adac_token.c
global-incdirs-y += psa-adac/sda/include
srcs-y += psa-adac/sda/src/psa_adac_sda.c
global-incdirs-y += transport_layer/transports
srcs-y += transport_layer/transports/static_buffer_msg.c
srcs-y += psa_adac_crypto.c

cflags-lib-y += -Wno-cast-align \
		-Wno-missing-prototypes \
		-Wno-missing-declarations \
		-Wno-old-style-definition \
		-Wno-declaration-after-statement \
		-Wno-unused-parameter \
		-Wno-sign-compare \
		-Wno-strict-aliasing \
		-Wno-unused-value
