<?php

/**
 * Функция автодополнения по начальным символам поиска
 * @param $params такого вида: title=тест&entity_type=node&bundle=vs
 */
function global_tree_search_callback($params) {
  $options = explode("&", $params);
  $conditions = array();
  foreach ($options as $option) {
    $condition = explode("=", $option);
    $conditions[trim($condition[0])] = trim($condition[1]); //key=>value
  }

  //Основные условия для выборки из БД
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', $conditions['entity_type'])
    ->entityCondition('bundle', $conditions['bundle'])
    ->propertyCondition('status', NODE_PUBLISHED)
    ->propertyCondition('title', '%' . $conditions['title'] . '%', 'like')
    ->propertyOrderBy('title', 'ASC')
    ->range(0, 10);

  //Дополнительные условия для выборки из БД
  foreach ($conditions as $key=>$value) {
    if ($key!='bundle' && $key!='entity_type' && $key!='title'){

      // Поиск по значению
      if (substr($value, 0,1)=='_'){
        $query->fieldCondition($key, 'value', substr($value,1));
      }

      // Поиск по TID
      elseif(substr($value, 0,3)=='tid'){
        $query->fieldCondition($key, 'tid', substr($value,3));
      }

      // Поиск по NID
      else{
        $query->fieldCondition($key, 'nid', $value, '=');
      }
    }
  }

  $result = $query->execute();
  $list = array();
  foreach ($result[$conditions['entity_type']] as $nid => $node) {
    $wrapper = entity_metadata_wrapper($conditions['entity_type'], $nid);
    $list[$nid] = $wrapper->label();
  }
  drupal_add_http_header('Content-Type', 'application/json');
  echo json_encode($list);
}
