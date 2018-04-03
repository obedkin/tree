<?php

/**
 * Функция AJAX - сортировки элементов дерева
 * @param $str_ids Пример: field_weight&field_ln_parent&tree-element[9613]=null&tree-element[9607]=9613
 * 1-й: поле веса для сортировки
 * 2-й: поле для связи с родительскими объектами
 * 3-й и т.д. tree-element[9613]=9613 id объекта и id его родителя. Если родителя нет, то NULL
 * Элементы идут уже в правильном порядке для распложения по сортировке
 */

function global_tree_sortable_callback() {

  // Поле сортировки
  if (isset($_POST['order_field'])) {
    $field_sortable_name = $_POST['order_field'];
  }
  else {
    exit('Не передано поле по которому необходимо осуществлять сортировку');
  }

  // Связь с родителями
  if (isset($_POST['parent_field'])) {
    $field_parent_name = $_POST['parent_field'];
  }
  else {
    exit('Не передано поле связи с родительским объектом');
  }

  // Корневой элемент (забыл для чего он)
  if (isset($_POST['root_id'])) {
    $root_id = preg_replace('~[^0-9]+~', '', $_POST['root_id']);
  }
  else {
    exit('Не передан ID корневого элемента');
  }

  // Если есть ID элементов
  if (isset($_POST['serialize'])) {
    $objects = array();
    $serialize = explode('&', $_POST['serialize']);
    //Парсим такую строку: i[9607]=9588 где 9607-потомок, 9588 - родитель, может быть NULL
    foreach ($serialize as $index => $str) {
      $id = substr($str, strpos($str, '[') + 1, strpos($str, ']') - strpos($str, '[') - 1);
      $parent_id = substr($str, strpos($str, '=') + 1);
      if ($parent_id == 'null') {
        if ($root_id > 0) {
          $parent_id = $root_id;
        }
        else {
          $parent_id = NULL;
        }
      }
      $objects[] = array(
        'id' => $id,
        'parent_id' => $parent_id,
        'index' => $index
      );
    }

    //Расставляем порядок и сортировку по правильным местам
    foreach ($objects as $index => $object) {
      $entity_wrapper = entity_metadata_wrapper('node', $object['id']);
      $entity_wrapper->$field_sortable_name->set($object['index']);   // Порядок сортировки

      // Если это обычный линейный список (field_parent_name = 0)
      if ($field_parent_name) {
        $entity_wrapper->$field_parent_name->set($object['parent_id']);
      }

      $entity_wrapper->save();
    }
  }

  echo 'Дерево успешно сохранено';
}
