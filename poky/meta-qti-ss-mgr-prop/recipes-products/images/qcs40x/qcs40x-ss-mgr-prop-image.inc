# SS-MGR Proprietary Packages
IMAGE_INSTALL += "${@base_conditional('USER_BUILD', '1', '', 'diag-reboot-app', d)}"
