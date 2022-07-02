COMMONS= ./instalar_commons.sh
REPOSITORIO= ./scripts/clonar_repositorio.sh

champagne:
	/bin/sh $(COMMONS) && /bin/sh $(REPOSITORIO)
