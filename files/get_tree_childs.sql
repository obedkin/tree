DROP PROCEDURE IF EXISTS `SAMPLE_DATABASE_NAME`.`GetTreeChilds`//

CREATE PROCEDURE `SAMPLE_DATABASE_NAME`.`GetTreeChilds` (IN GivenID INT, IN field_name VARCHAR(256), IN tbl_prefix VARCHAR(256), IN field_suffix VARCHAR(256), IN bundle VARCHAR(256))
BEGIN
  DECLARE q,queue,queue_children LONGTEXT;
  DECLARE queue_length,pos LONG;
  DECLARE front_id LONG;
	DECLARE tbl_name VARCHAR (256);
	DECLARE value_name VARCHAR (256);
	DECLARE child_ids LONGTEXT;

	SET child_ids = "";
	SET queue = GivenID;
	SET queue_length = 1;

	SET tbl_name = CONCAT(tbl_prefix, "field_data_", field_name);
	SET value_name = CONCAT(field_name, "_", field_suffix);

	WHILE queue_length > 0 DO
	  SET front_id = queue;

	  IF queue_length = 1 THEN
	    SET queue = "";
	  ELSE
      SET pos = LOCATE(",",queue) + 1;
      SET q = SUBSTR(queue,pos);
      SET queue = q;
    END IF;

		SET queue_length = queue_length - 1;
    SET @temp_queue_children = queue_children;
		SET @SQL = CONCAT("SELECT IFNULL(qc, -1) INTO @temp_queue_children FROM (SELECT GROUP_CONCAT(entity_id) qc FROM `",tbl_name,"` WHERE `bundle` = \'",bundle,"\' AND `",value_name,"` = \'",front_id,"\') A;");

		PREPARE stmt FROM @SQL;
		EXECUTE stmt;
		DEALLOCATE PREPARE stmt;

		SET queue_children = @temp_queue_children;

    IF queue_children = -1 THEN
      IF LENGTH(queue) = 0 THEN
        SET queue_length = 0;
      END IF;
    ELSE
      IF LENGTH(child_ids) = 0 THEN
        SET child_ids = queue_children;
      ELSE
        SET child_ids = CONCAT(child_ids,",",queue_children);
      END IF;

      IF LENGTH(queue) = 0 THEN
        SET queue = queue_children;
      ELSE
        SET queue = CONCAT(queue,",",queue_children);
      END IF;

      SET queue_length = LENGTH(queue) - LENGTH(REPLACE(queue,",","")) + 1;
    END IF;
  END WHILE;

  SELECT child_ids;
END//
