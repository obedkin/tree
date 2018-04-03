<?php
/**
 * Малая форма для добавления новых элементов в дерево. У элемента указывается только название
 * @param $form
 * @param $form_state
 * @param $parent_id
 * @param $options
 * @return mixed
 */
function global_tree_form_add($form, &$form_state, $options) {

  // TODO: Потом аналогичным образом перейти на action для добавления и редактирования
  if (isset($options['action']) && $options['action'] == 'copy') {
    $node_wrapper = entity_metadata_wrapper($options['entity_type'], $options['id']);
    $title = $node_wrapper->label();
    $type = '_copy';
    $icon = '<i class="fa fa-files-o fa-submit"></i>';
    $icon_class = 'btn-primary';
  }
  else {
    //Если есть ID, значит форма редактирования, а не добавления
    if (isset($options['id'])) {
      $node_wrapper = entity_metadata_wrapper($options['entity_type'], $options['id']);
      $title = $node_wrapper->label();
      $type = '_edit';
    }
    else {
      $title = '';
      $type = '_add';
    }
    $icon = '<i class="fa fa-plus-circle fa-submit"></i>';
    $icon_class = 'btn-success';
  }

  $form['#attributes']['class'][] = 'form-add';
  $form['group'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('input-group'),
    ),
  );
  $form['group']['title'] = array(
    '#type' => 'textfield',
    '#title_display' => FALSE,
    '#default_value' => $title,
    '#attributes' => array(
      'placeholder' => 'Наименование',
      'class' => array('input-small'),
    ),
  );


  $form['group']['save'] = array(
    '#type' => 'submit',
    '#value' => '',
    '#attributes' => array('style' => array('display: none')),
    '#prefix' => '<span class="span-submit ' . $type . '">' . $icon,
    '#suffix' => '</span>',
    '#ajax' => array(
      'callback' => 'global_tree_form_add_ajax_callback',
      'event' => 'click'
    ),
  );

  $form['options'] = array('#type' => 'value', '#value' => $options,);
  return $form;
}


/**
 * Универсальный ajax-callback для универсальной формы (добавления, редактирования и копирования)
 * @param $form
 * @param $form_state
 * @return array
 */
function global_tree_form_add_ajax_callback($form, &$form_state) {

  $options = $form_state['values']['options'];//Все опции дерева
  unset($form_state['values']['options']);
  $options['values'] = $form_state['values'];//Значения из формы добавления

  $after_submit_args = array();

  if (!empty($options['action'])) {
    $action = $options['action'];
  }
  elseif (!empty($options['id'])) {
    $action = 'edit';
  }
  else {
    $action = 'add';
  }

  // TODO: Потом перейти на action для добавления и редактирования
  // Копирование
  if ($action == 'copy') {

    $donor_id = $options['root_id'];

    // Абстрактная функция копирования. Может быть переопределена
    $node = $options['class']::copy($options);
    $options['root_id'] = $node['nid'];
    $new_id = $node['nid'];
    $options['level']--;

    // Новый объект дочернего класса
    $tree = new $options['class']($options);
    $tree_item = $tree->get_tree_item($node);
    $commands = array();
    $commands[] = ajax_command_after('#i-' . $donor_id, '<li id="' . $tree_item['id'] . '" class="' . implode(" ", $tree_item['class']) . '">' . $tree_item['data'] . '</li>');
    $commands[] = ajax_command_invoke('#div-form-copy-' . $donor_id, 'removeClass', array('in'));
    $commands[] = ajax_command_invoke('#dif-form-copy-' . $donor_id, 'addClass', array('collapse'));
  }
  // Редактирование
  elseif ($action == 'edit') {
    $options['class']::edit($options);

    $form_state['build_info']['args'] = array($options);
    $form_state['values'] = array();
    $form_state['input'] = array();

    if (!$options['root_id']) {
      $options['level'] = NULL;
    }

    $commands[] = ajax_command_html('#item_href_' . $options['id'], $options['values']['title']); //Обновляем элемент
    $form_rebuild = drupal_build_form('global_tree_form_add', $form_state);
    array_unshift($commands, ajax_command_replace('#form-add-' . $options['root_id'], drupal_render($form_rebuild)));
    array_unshift($commands, ajax_command_invoke('#div-form-edit-' . $options['root_id'], 'removeClass', array('in')));//Прячем все дополнительные формы
    array_unshift($commands, ajax_command_invoke('#div-form-edit-' . $options['root_id'], 'addClass', array('collapse')));//Прячем все дополнительные формы

    //Изменяем атрибуты у самого элемента
    $tree = new $options['class']($options);
    $attr = $tree->get_tree_item_attr();
    $commands[] = ajax_command_html('#item-attributes-' . $options['root_id'], $attr);

    $after_submit_args = array($options);
  }
  // Добавление
  elseif ($action == 'add') {
    $new_id = $options['class']::add($options);

    $form_state['build_info']['args'] = array($options);
    $form_state['values'] = array();
    $form_state['input'] = array();

    // Проверка на корневой элемент дерева
    if (!$options['root_id'] || $options['root_id'] == $options['root']
      //|| (is_array($options['root']) && ($options['root'][0] == $options['root_id']))
    ) {
      $options['level'] = NULL;
    }

    $commands = global_tree_load($options);//Загружаем это дерево

    // Условие, если многокорневое дерево состоящее из 1 корня
    if ((is_array($options['root']) && ($options['root'][0] == $options['root_id']))) {
      array_unshift($commands, ajax_command_remove("#i-" . $options['root_id'] . ' #tree-' . $options['root_id']));//Удаляем старое дерево
    }

    // Если добавление происходит в активный (target_id) элемент дерева. То удаляем этот блок
    if ($options['root_id'] == $options['target_id']) {
      $commands[] = ajax_command_remove('ul#i-' . $options['target_id']);
    }

    if ($options['root_id'] && $options['root_id'] != $options['root'] && !is_array($options['root'])) {//Если это не корневой элемент дерева
      array_unshift($commands, ajax_command_remove("#tree-" . $options['root_id']));//Удаляем старое дерево
      $commands[] = ajax_command_invoke('#i-' . $options['root_id'], 'removeClass', array('empty'));//Ставим стрелочку окола родительского элемента

      //Изменяем атрибуты у родительского элемента
      $tree = new $options['class']($options);
      $attr = $tree->get_tree_item_attr();
      $commands[] = ajax_command_html('#item-attributes-' . $options['root_id'], $attr);
    }

    $form_rebuild = drupal_build_form('global_tree_form_add', $form_state);
    array_unshift($commands, ajax_command_replace('#form-add-' . $options['root_id'], drupal_render($form_rebuild)));
    array_unshift($commands, ajax_command_invoke('#div-form-add-' . $options['root_id'], 'addClass', array('collapse')));//Прячем все дополнительные формы
    array_unshift($commands, ajax_command_invoke('#div-form-add-' . $options['root_id'], 'removeClass', array('in')));//Прячем все дополнительные формы

    $after_submit_args = array($new_id, $options);
  }

  // Выполняем функции, которые должны выполняться после ajax-submit (опционально)
  if (isset($options[$action]['after_submit']) && function_exists($options[$action]['after_submit'])) {
    $commands_after_submit = call_user_func_array($options[$action]['after_submit'], $after_submit_args);
    $commands = array_merge($commands, $commands_after_submit);
  }

  // Делаем неактивными все элементы хоть при добавлении, хоть при копировании
  $commands[] = ajax_command_invoke('.tree_btn', 'removeClass', array('active'));

  // Останавливаем все крутилки
  $commands[] = ajax_command_invoke('.fa-spin', 'removeClass', array('fa-spin'));

  return array("#type" => 'ajax', "#commands" => $commands);
}

function global_tree_form_add_submit($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
}
