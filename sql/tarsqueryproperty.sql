-- MySQL dump 10.13  Distrib 5.1.51, for Win32 (ia32)
--
-- Host: localhost    Database: db_tars
-- ------------------------------------------------------
-- Server version	5.1.51-community

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

--
-- Dumping data for table `t_server_conf`
--
-- WHERE:  server_name='tarsqueryproperty'

LOCK TABLES `t_server_conf` WRITE;
/*!40000 ALTER TABLE `t_server_conf` DISABLE KEYS */;
INSERT INTO `t_server_conf` VALUES (27,'tars','tarsqueryproperty','','10.120.129.226','2017-12-01 06:51:22','','/usr/local/app/tars/tarsqueryproperty/bin/tarsqueryproperty','tars.tarsproperty',0,'active','active',6405,'57','2017-01-04 16:48:32','','1.0.1','2017-01-16 09:56:55',NULL,'tars_cpp','','','','N','N',NULL,NULL,NULL,NULL,'<tars>\n<countdb>\n<db1>\n			dbhost=db.tars.com\n			dbname=tars_property\n			tbname=tars_property_\n			dbuser=tars\n			dbpass=tars2015\n			dbport=3306\n			charset=utf8\n		</db1>\n</countdb>\n</tars>',0,3,'0','65','2',0);
/*!40000 ALTER TABLE `t_server_conf` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `t_adapter_conf`
--
-- WHERE:  server_name='tarsqueryproperty'

LOCK TABLES `t_adapter_conf` WRITE;
/*!40000 ALTER TABLE `t_adapter_conf` DISABLE KEYS */;
INSERT INTO `t_adapter_conf` VALUES (28,'tars','tarsqueryproperty','10.120.129.226','tars.tarsqueryproperty.NoTarsObjAdapter','2017-01-04 08:48:16',5,'tcp -h 10.120.129.226 -t 60000 -p 10011',200000,'','tars.tarsqueryproperty.NoTarsObj',10000,60000,'2017-01-04 16:46:41',NULL,'not_tars','');
/*!40000 ALTER TABLE `t_adapter_conf` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2018-03-06 14:49:36
