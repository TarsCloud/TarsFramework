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
-- WHERE:  server_name='tarsconfig'

LOCK TABLES `t_server_conf` WRITE;
/*!40000 ALTER TABLE `t_server_conf` DISABLE KEYS */;
INSERT INTO `t_server_conf` VALUES (2,'tars','tarsconfig','','10.120.129.226','2017-12-01 06:51:29','','/usr/local/app/tars/tarsconfig/bin/tarsconfig','tars.tarsconfig',0,'active','active',28007,'','0000-00-00 00:00:00','','1.0.1','2015-08-08 10:49:37','admin','tars_cpp','','','','N','N','','','','','',0,3,'0','65','2',0,'NORMAL');
/*!40000 ALTER TABLE `t_server_conf` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `t_adapter_conf`
--
-- WHERE:  server_name='tarsconfig'

LOCK TABLES `t_adapter_conf` WRITE;
/*!40000 ALTER TABLE `t_adapter_conf` DISABLE KEYS */;
INSERT INTO `t_adapter_conf` VALUES (2,'tars','tarsconfig','10.120.129.226','tars.tarsconfig.ConfigObjAdapter','2015-11-28 16:16:26',10,'tcp -h 10.120.129.226 -t 60000 -p 10001',200000,'','tars.tarsconfig.ConfigObj',10000,60000,'2015-11-28 11:16:26','admin','tars','');
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

-- Dump completed on 2018-03-06 14:47:10
