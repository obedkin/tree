DROP PROCEDURE IF EXISTS `SAMPLE_DATABASE_NAME`.`GetTreeParents`//

CREATE PROCEDURE `SAMPLE_DATABASE_NAME`.`GetTreeParents` (IN GivenID INT, IN field_name VARCHAR(256), IN tbl_prefix VARCHAR(256), IN field_suffix VARCHAR(256))
BEGIN
  DECLARE cm CHAR(1);
  DECLARE ch INT;
	DECLARE tbl_name VARCHAR (256);
	DECLARE value_name VARCHAR (256);
	DECLARE parents_ids LONGTEXT;

	SET cm = '';
  SET ch = GivenID;
  SET parents_ids = '';

  SET tbl_name = CONCAT(tbl_prefix, "field_data_", field_name);
	SET value_name = CONCAT(field_name, "_", field_suffix);


	WHILE ch > 0 DO
	  SET @temp_ch = ch;
		SET @SQL = CONCAT('SELECT IFNULL(',value_name,',-1) INTO @temp_ch FROM (SELECT MIN(COALESCE(',value_name,',-1)) as ',value_name,' FROM ',tbl_name,' WHERE entity_id = @temp_ch) A;');

		PREPARE stmt FROM @SQL;
		EXECUTE stmt;
		DEALLOCATE PREPARE stmt;

		SET ch = @temp_ch;

		IF ch > 0 THEN
      SET parents_ids = CONCAT(parents_ids,cm,ch);
      SET cm = ',';
    END IF;
  END WHILE;

  SELECT parents_ids;
END//
