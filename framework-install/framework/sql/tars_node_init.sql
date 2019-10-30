-- MySQL dump 10.13  Distrib 5.6.26, for Linux (x86_64)
--
-- Host: localhost    Database: db_tars
-- ------------------------------------------------------
-- Server version	5.6.26-log

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

LOCK TABLES `t_node_info` WRITE;
/*!40000 ALTER TABLE `t_node_info` DISABLE KEYS */;

insert into `t_node_info` (`node_name`, `node_obj` , `endpoint_ip`, `endpoint_port`, `data_dir`,  `last_reg_time` ,  `last_heartbeat`,`setting_state`, `present_state`, `tars_version`, `template_name`, `modify_time`, `group_id`) VALUES ('192.168.2.131','tars.tarsnode.NodeObj@tcp -h 192.168.2.131 -p 19385 -t 60000','192.168.2.131',19385,'/usr/local/app/tars/tarsnode/data',now(),now(),'active','active','1.0.1','',now(),-1);
/*!40000 ALTER TABLE `t_node_info` ENABLE KEYS */;
UNLOCK TABLES;


LOCK TABLES `t_registry_info` WRITE;
/*!40000 ALTER TABLE `t_registry_info` DISABLE KEYS */;
insert into `t_registry_info` (`locator_id`,`servant`,`endpoint`,`last_heartbeat`,`present_state`,`tars_version`) VALUES ('192.168.2.131:12000','taf.tafAdminRegistry.AdminRegObj','tcp -h 192.168.2.131 -p 12000 -t 60000',now(),'active','1.0.0'),('192.168.2.131:17890','taf.tafregistry.QueryObj','tcp -h 192.168.2.131 -p 17890 -t 10000',now(),'active','1.0.0'),('192.168.2.131:17890','taf.tafregistry.RegistryObj','tcp -h 192.168.2.131 -p 17891 -t 30000',now(),'active','1.0.0')

/*!40000 ALTER TABLE `t_registry_info` ENABLE KEYS */;
UNLOCK TABLES;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2016-11-24 14:28:02
