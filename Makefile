SERVICE=mode_agent
SERVICE_CLI=mode_ctrl_cli
SRVC_MGMT_SCRIPT=mode_agent_mgmt
SRVC_CFG=mode_ctrl_config

CURRENT_DIR=$(shell pwd)
TARGET_SRVC_INSTALL_DIR=/sbin
TARGET_CLI_INSTALL_DIR=/usr/bin
TARGET_INSTALL_SCRIPT_DIR=/etc/init.d
TARGET_INSTALL_SRVC_CFG_DIR=/etc

all:
	cd service && make && cd ..
	cd ui && make && cd ..

install:
	cp $(CURRENT_DIR)/scripts/$(SRVC_MGMT_SCRIPT) $(TARGET_INSTALL_SCRIPT_DIR)
	cp $(CURRENT_DIR)/scripts/$(SRVC_CFG) $(TARGET_INSTALL_SRVC_CFG_DIR)
	cp $(CURRENT_DIR)/ui/$(SERVICE_CLI) $(TARGET_CLI_INSTALL_DIR)
	cp $(CURRENT_DIR)/service/$(SERVICE) $(TARGET_SRVC_INSTALL_DIR)

uninstall:
	@echo "Конфигурационный файл ${TARGET_INSTALL_SRVC_CFG_DIR}/${SRVC_CFG} необходимо удалить вручную"
	rm -f $(TARGET_INSTALL_SCRIPT_DIR)/$(SRVC_MGMT_SCRIPT)
	rm -f $(TARGET_CLI_INSTALL_DIR)/$(SERVICE_CLI)
	rm -f $(TARGET_SRVC_INSTALL_DIR)/$(SERVICE)
