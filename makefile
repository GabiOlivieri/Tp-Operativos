COMMONS= ./instalar_commons.sh
REPOSITORIO= ./scripts/clonar_repositorio.sh
ACTUALIZADOR= ./actualizar_ips_locales.sh

deploy:
	/bin/sh $(COMMONS) && /bin/sh $(ACTUALIZADOR)
