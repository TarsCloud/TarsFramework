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
-- WHERE:  server_name='tarsproperty'

LOCK TABLES `t_server_conf` WRITE;
/*!40000 ALTER TABLE `t_server_conf` DISABLE KEYS */;
INSERT INTO `t_server_conf` VALUES (25,'tars','tarsproperty','','10.120.129.226','2017-12-01 06:51:21','','/usr/local/app/tars/tarsproperty/bin/tarsproperty','tars.tarsproperty',0,'active','active',9393,'55','2017-01-04 16:43:27','','1.0.1','2017-01-04 16:42:40',NULL,'tars_cpp',NULL,NULL,NULL,'N','N',NULL,NULL,NULL,NULL,NULL,0,3,'0','65','2',0,'NORMAL');
/*!40000 ALTER TABLE `t_server_conf` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `t_adapter_conf`
--
-- WHERE:  server_name='tarsproperty'

LOCK TABLES `t_adapter_conf` WRITE;
/*!40000 ALTER TABLE `t_adapter_conf` DISABLE KEYS */;
INSERT INTO `t_adapter_conf` VALUES (26,'tars','tarsproperty','10.120.129.226','tars.tarsproperty.PropertyObjAdapter','2017-01-04 08:42:40',5,'tcp -h 10.120.129.226 -t 60000 -p 10005',200000,'','tars.tarsproperty.PropertyObj',10000,60000,'2017-01-04 16:42:40',NULL,'tars','');
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

-- Dump completed on 2018-03-06 14:49:07
