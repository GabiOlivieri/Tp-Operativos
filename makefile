COMMONS= ./instalar_commons.sh
MAKER= ./scripts/makeAllFiles.sh

deploy:
	/bin/sh $(COMMONS) && /bin/sh $(MAKER)
