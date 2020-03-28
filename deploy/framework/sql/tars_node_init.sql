-- MySQL dump 10.13  Distrib 5.6.26, for Linux (x86_64)
--
-- Host: localhost    Database: db_tars
-- ------------------------------------------------------
-- Server version	5.6.26-log

replace into `t_node_info` (`node_name`, `node_obj` , `endpoint_ip`, `endpoint_port`, `data_dir`,  `last_reg_time` ,  `last_heartbeat`,`setting_state`, `present_state`, `tars_version`, `template_name`, `modify_time`, `group_id`) VALUES ('localip.tars.com','tars.tarsnode.NodeObj@tcp -h localip.tars.com -p 19385 -t 60000','localip.tars.com',19385,'/usr/local/app/tars/tarsnode/data',now(),now(),'active','active','2.1.0','',now(),-1);
replace into `t_registry_info` (`locator_id`,`servant`,`endpoint`,`last_heartbeat`,`present_state`,`tars_version`) VALUES ('localip.tars.com:12000','tars.tarsAdminRegistry.AdminRegObj','tcp -h localip.tars.com -p 12000 -t 60000',now(),'active','1.0.0'),('localip.tars.com:17890','tars.tarsregistry.QueryObj','tcp -h localip.tars.com -p 17890 -t 10000',now(),'active','1.0.0'),('localip.tars.com:17890','tars.tarsregistry.RegistryObj','tcp -h localip.tars.com -p 17891 -t 30000',now(),'active','2.1.0');


-- Dump completed on 2016-11-24 14:28:02
