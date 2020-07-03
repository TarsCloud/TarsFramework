-- MySQL dump 10.13  Distrib 5.6.26, for Linux (x86_64)
--
-- Host: localhost    Database: db_tars
-- ------------------------------------------------------
-- Server version       5.6.26-log

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

-- tarslog
replace into `t_adapter_conf` (`application`, `server_name`, `node_name`, `adapter_name`,`registry_timestamp`, `thread_num`, `endpoint`, `max_connections`, `allow_ip`, `servant`, `queuecap`, `queuetimeout`,`posttime`,`lastuser`,`protocol`, `handlegroup`) VALUES ('tars','tarslog','localip.tars.com','tars.tarslog.LogObjAdapter',now(),5,'tcp -h localip.tars.com -t 60000 -p 18993',200000,'','tars.tarslog.LogObj',200000,60000,now(),NULL,'tars','');

replace into `t_server_conf` (`application`, `server_name`, `node_group`, `node_name`, `registry_timestamp`, `base_path`, `exe_path`, `template_name`, `bak_flag`, `setting_state`, `present_state`, `process_id`, `patch_version`, `patch_time`, `patch_user`, `tars_version`, `posttime`, `lastuser`, `server_type`) VALUES ('tars','tarslog','','localip.tars.com',now(),'','/usr/local/app/tars/tarslog/bin/tarslog','tars.tarslog',0,'active','active',0,'2.1.0',now(),'','2.1.0',now(),'admin','tars_cpp');

/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2016-11-24 14:28:02
