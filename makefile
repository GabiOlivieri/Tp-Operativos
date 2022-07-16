COMMONS= ./instalar_commons.sh
MAKER= ./scripts/makeAllFiles.sh
PRUEBAS = ./scripts/clonar_pruebas.sh

deploy:
	/bin/sh $(COMMONS) && /bin/sh $(MAKER) && /bin/sh $(PRUEBAS) 
