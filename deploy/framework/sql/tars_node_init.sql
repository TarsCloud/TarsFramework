-- MySQL dump 10.13  Distrib 5.6.26, for Linux (x86_64)
--
-- Host: localhost    Database: db_tars
-- ------------------------------------------------------
-- Server version	5.6.26-log

replace into `t_node_info` (`node_name`, `node_obj` , `endpoint_ip`, `endpoint_port`, `data_dir`,  `last_reg_time` ,  `last_heartbeat`,`setting_state`, `present_state`, `tars_version`, `template_name`, `modify_time`, `group_id`) VALUES ('localip.tars.com','tars.tarsnode.NodeObj@tcp -h localip.tars.com -p 19385 -t 60000','localip.tars.com',19385,'/usr/local/app/tars/tarsnode/data',now(),now(),'active','active','2.1.0','',now(),-1);

-- Dump completed on 2016-11-24 14:28:02
