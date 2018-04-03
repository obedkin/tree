<?php

/**************************************************************************************
 * Универсальный класс по представлению взаимосвязанных данных в виде дерева
 * date: 18.02.2015
 * description: В дочерних классах необходимо переопределять функции
 *    get_tree_res: если выборка будет проходить по нескольким нетипичным полям
 *    global_tree_query_has_not_parent_alter: дополнительный JOIN для корневых элементов
 **************************************************************************************/
class GlobalTree {

  protected $_options = array();    // Все опции для построения дерева
  protected $_childs_count = NULL;  // Кол-во детей

  /**
   * @param $options
   */
  public function __construct($options) {

    $this->_options = $options;

    // Название родительского или переопределенного дочернего класса.
    // Нужно будет для AJAX-методов
    $this->_options['class'] = get_class($this);

    // Название ID типа данных вычисляем в зависимости от типа сущности, если оно
    // Не было принудительно передано через конструктор
    // Т.е. для ноды возможны 2 вида type_id: nid и target_id
    if (!isset($this->_options['entity_type_id'])) {
      $this->_options['entity_type_id'] = 'nid';
    }

    if (isset($options['target_id']) && !isset($options['parents'])) {
      $this->_options['parents'] = $this->get_parents($options['target_id']);
      // Расскрываем и сам искомый элемент дерева
      $this->_options['parents'][] = $options['target_id'];
    }

    //Уровни дерева для стилизации
    if (isset($this->_options['level'])) {
      $this->_options['level']++;
    }
    else {
      $this->_options['level'] = 1;
      if (isset($this->_options['root_id'])) {
        $this->_options['root'] = $this->_options['root_id']; // Самый главный корень
      }
      else {
        $this->_options['root'] = NULL;
      }

    }

    //Название функции формы добавления (Не передана || Передана, но не существует)
    if (!isset($options['function_form_add']) || (isset($options['function_form_add']) && !function_exists($options['function_form_add']))) {
      $this->_options['function_form_add'] = 'global_tree_form_add';//Иначе функция по умолчанию
    }
  }


  /**
   * Фуникция отрисовки html-дерева
   * @return string
   * @throws \Exception
   */
  public function get_tree_html() {
    $this->init_static_content();

    $items = array();

    //Если многокорневое дерево
    if (isset($this->_options['root_id']) && is_array($this->_options['root_id'])) {
      $root_ids = $this->_options['root_id'];
      foreach ($root_ids as $root_id) {
        $this->_options['root_id'] = $root_id;
        if (!isset($this->_options['root_show']) || $this->_options['root_show']) {
          $entity = entity_metadata_wrapper($this->_options['entity_type'], $this->_options['root_id']);
          $node['title'] = $entity->label();
          $node['nid'] = $entity->getIdentifier();

          //Есть дети у многокорневого дерева или нет
          $options = $this->_options;
          $options['root_id'] = $entity->getIdentifier();
          // Для проверки тестового комита
          $childs = $options['class']::get_childs($options);
          if ($childs) {
            $node['child_count'] = count($childs);
          }
          else {
            $node['child_count'] = 0;
          };

          //Итоговые элементы дерева
          $items[] = $this->get_tree_item($node);
        }
        else {
          $items = array_merge($items, $this->get_tree_items());
        }
      }
      $this->_options['root_id'] = $root_ids;
    }
    else {
      $items = array_merge($items, $this->get_tree_items());
    }

    //Если многокорневое дерево, скдываем ROOT_ID, т.к. ID должен быть уникальным для дерева
    if (isset($this->_options['root_id']) && is_array($this->_options['root_id']) && count($this->_options['root_id']) > 1) {
      $this->_options['root_ids'] = $this->_options['root_id'];
      $this->_options['root_id'] = NULL;
    }

    if (isset($this->_options['root_id'])) {
      if (is_array($this->_options['root_id'])) {
        $root_id = $this->_options['root_id'][0];
      }
      else {
        $root_id = $this->_options['root_id'];
      }
    }
    else {
      $root_id = '';
    }

    // Устанавливаем возможность сортировки

    $ul_class = array('tree');
    $ul_data_order_field = '';
    $ul_data_parent_field = '';
    if (isset($this->_options['order']) && isset($this->_options['order']['field'])) {
      $ul_class[] = 'sortable';
      $ul_data_order_field = $this->_options['order']['field']; // Наименование поля для сортировки по весу
      $ul_data_parent_field = 0;
      // Если не указано родительское поле, то это обычный линейный список
      if (isset($this->_options['parent_field'])) {
        $ul_data_parent_field = $this->_options['parent_field'];
      }
    }

    $tree_html = theme('item_list', array(
      'items' => $items,
      'attributes' => array(
        'id' => 'tree-' . $root_id,
        'class' => $ul_class,
        'data-order-field' => $ul_data_order_field,
        'data-parent-field' => $ul_data_parent_field,
      )
    ));

    // Почему-то иногда THEME оборачивает <UL> дополнительным DIV, есть
    // примеры где такого оборачивания не происходит
    $tree_html = str_replace('<div class="item-list">', "", $tree_html);
    $tree_html = str_replace('</ul></div>', "</ul>", $tree_html);

    // Если это пустое дерево, т.е. не было связки </ul></div>
    if ($tree_html == '</div>') {
      $tree_html = '';
    }

    // Добавляем кнопки управления деревом, если дерево не пустое
    $top_buttons = '';
    if ($tree_html == '') {
      $top = $this->get_top_buttons();
      $tree_html = $top . '<div id="tree-' . $root_id . '"><div class="alert alert-info" role="alert">Нет элементов для отображения</div></div>';
    }
    else {
      if (!isset($this->_options['root_id']) || $this->_options['root_id'] == '' || is_array($this->_options['root_id']) || $this->_options['level'] == 1) {
         $top_buttons = $this->get_top_buttons();
      }
    }

    return $top_buttons . $tree_html;
  }


  /**
   * Кнопки сверху дерева (Добавить, Раскрыть, Свернуть)
   * @return array На выходе объект типа theme_item_list который будет идти первым элементом при построении дерева
   */
  protected function get_top_buttons() {
    $add = $this->get_top_add();        // Кнопка добавить корневой элемент в дерево
    $close = $this->get_top_close();    // Кнопка свернуть всё дерево
    $open = $this->get_top_open();      // Кнопка развернуть всё дерево
    $sort = $this->get_top_sort();      // Кнопка сортировки
    $search = $this->get_top_search();  // Строка поиска
    $check = $this->get_top_check();    // Кнопка с галочками
    return "<div class='tree-top'>" . $search . "<div class='top-buttons'>" . $add['icon'] . $close . $open . $sort . $check . "</div>" . $add['form'] . "</div>";
  }


  /**
   * Кнопка и форма добавления корневых элементов
   * @return array ['icon','form'] иконка и форма
   */
  public function get_top_add() {
    $form_add = array('icon' => '', 'form' => '');
    if (!is_array($this->_options['root']) && ((isset($this->_options['admin']) && ($this->_options['admin']) && !isset($this->_options['top']['buttons'])) || (isset($this->_options['top']['buttons']) && in_array("add", $this->_options['top']['buttons'])))) {
      if (!isset($this->_options['root_id'])) {
        $this->_options['root_id'] = NULL;
      }
      $form_add = $this->get_add_form($this->_options['root_id']);
    }
    return $form_add;
  }


  /**
   * Кнопка "Свернуть всё дерево"
   * @return string
   */
  public function get_top_close() {
    $icon_close = '';

    // Если есть элементы для отображения
    if ($this->_childs_count) {
      if (!isset($this->_options['top']['buttons']) || (isset($this->_options['top']['buttons']) && is_array($this->_options['top']['buttons']) && in_array("close", $this->_options['top']['buttons']))) {
        if (isset($this->_options['top']['show_title']) && $this->_options['top']['show_title'] == FALSE) {
          $title = '';
        }
        else {
          $title = 'Свернуть';
        }
        $query = $this->_options;
        $query['f'] = 'global_tree_close';
        $icon_close = l($title, 'l/ajax', array(
          'query' => $query,
          'html' => TRUE,
          'attributes' => array(
            'class' => array('tree_btn', 'btn_close', 'use-ajax'),
            'title' => 'Свернуть дерево'
          ),
        ));
      }
    }
    return $icon_close;
  }


  /**
   * Кнопка "Развернуть всё дерево"
   * @return string
   */
  public function get_top_open() {
    $icon_open = '';

    // Если есть элементы для отображения
    if ($this->_childs_count) {
      if (!isset($this->_options['top']['buttons']) || (isset($this->_options['top']['buttons']) && is_array($this->_options['top']['buttons']) && in_array("open", $this->_options['top']['buttons']))) {
        if (isset($this->_options['root_ids'])) {
          $this->_options['root_id'] = $this->_options['root_ids'];
        }
        if (isset($this->_options['top']['show_title']) && $this->_options['top']['show_title'] == FALSE) {
          $title = '';
        }
        else {
          $title = 'Развернуть';
        }
        $query = $this->_options;
        $query['f'] = 'global_tree_open';
        $icon_open = l($title, 'l/ajax', array(
          'query' => $query,
          'html' => TRUE,
          'attributes' => array(
            'class' => array('tree_btn', 'btn_open', 'use-ajax'),
            'title' => 'Развернуть дерево'
          ),
        ));
      }
    }
    return $icon_open;
  }

  /**
   * Кнопка сортировки элементов дерева
   * @return string
   */
  public function get_top_sort() {
    $icon_sort = '';

    // Если есть элементы для отображения
    if ($this->_childs_count) {
      if ((isset($this->_options['admin']) && $this->_options['admin']) && (!isset($this->_options['top']['buttons']) || (isset($this->_options['top']['buttons']) && is_array($this->_options['top']['buttons']) && in_array("sort",
              $this->_options['top']['buttons']) && isset($this->_options['order']['field'])))
      ) {
        if (isset($this->_options['top']['show_title']) && $this->_options['top']['show_title'] == FALSE) {
          $title = '<i class="fa fa-arrows"></i>';
        }
        else {
          $title = '<i class="fa fa-arrows"></i> Сортировать';
        }
        $icon_sort = l($title, '', array(
          'html' => TRUE,
          'attributes' => array(
            'class' => array('tree_btn', 'btn_sort'),
            'title' => 'Вкл/выкл сортировку'
          ),
        ));
      }
    }
    return $icon_sort;
  }

  /**
   * Строка ввода для поиска
   * @return string
   */
  public function get_top_search() {
    $search_html = '';
    // Если есть элементы для отображения
    if ($this->_childs_count) {
      if (isset($this->_options['search']) && $this->_options['search'] && isset($this->_options['url']) && $this->_options['url']) {
        drupal_add_library('system', 'ui.autocomplete');
        $search_html = $this->get_search_html();
      }
    }
    return $search_html;
  }


  /**
   * Кнопка в шапке дерева для последовательности обхода
   * @return string
   */
  public function get_top_check() {
    $icon_check = '';

    // Если есть элементы для отображения
    if ($this->_childs_count) {
      if (isset($this->_options['top']['buttons']['check'])) {
        $title = '<i class="fa fa-check-square-o"></i>';
        if (isset($this->_options['top']['show_title']) && $this->_options['top']['show_title'] == TRUE && isset($this->_options['top']['buttons']['check']['title'])) {
          $title .= ' ' . $this->_options['top']['buttons']['check']['title'];
        }
        $icon_check = l($title, '', array(
          'html' => TRUE,
          'attributes' => array(
            'class' => array(
              'tree_btn',
              'btn_check',
              (global_tree_check_show()) ? 'active' : ''
            ),
            'title' => (isset($this->_options['top']['buttons']['check']['description'])) ? $this->_options['top']['buttons']['check']['description'] : '',
            'data-out-selector' => (isset($this->_options['top']['buttons']['check']['out_selector'])) ? $this->_options['top']['buttons']['check']['out_selector'] : '',
          ),
        ));
      }
    }
    return $icon_check;
  }


  /**
   * Получение кол-ва дочерних элементов
   * @return integer количество дочерних элементов
   */
  public function get_childs_count() {
    return $this->_childs_count;
  }


  /**
   * Список элементов li для построения вложенного списка
   * https://api.drupal.org/api/drupal/includes!theme.inc/function/theme_item_list/7
   * @return array
   */
  protected function get_tree_items() {
    $res = $this->get_tree_res();
    $items = array();
    foreach ($res as $node) {
      $items[] = $this->get_tree_item($node);
    }
    return $items;
  }


  /**
   * Получение списка из вложенных элементов
   * Обычно нужно для SELECT. В результате будет массив такого вида
   *
   * Уровень 1
   * -- Уровень 1.1.
   * ---- Уровень 1.1.1
   * -- Уровень 1.2
   * Уровень 2
   * -- Уровень 2.1
   * ----Уровень 2.1.1
   *
   * @return array
   */
  public function get_tree_list() {
    // Если не указаны символы для отступа
    if (!isset($this->_options['indent'])) {
      $this->_options['indent'] = '--';
    }
    return $this->get_tree_list_items();
  }


  /**
   * Формирование одного элемента для вложенного списка
   * @return array на выходе массив вида  ID => '-- Уровень 1.1'
   */
  private function get_tree_list_items() {
    $res = $this->get_tree_res();
    $items = array();
    foreach ($res as $node) {
      $indent = '';
      for ($i = 2; $i <= $this->_options['level']; $i++) {
        $indent .= $this->_options['indent'];
      }
      $items[$node['nid']] = $indent . ' ' . $node['title'];
      $this->_options['root_id'] = $node['nid'];
      $this->_options['level']++;
      $child = $this->get_tree_list_items();
      if ($child) {
        $items = $items + $child;
      }
      $this->_options['level']--;
    }
    return $items;
  }


  /**
   * Объект для формирования одного элемента списка
   * https://api.drupal.org/api/drupal/includes!theme.inc/function/theme_item_list/7
   * @param $node
   * @return array
   */
  public function get_tree_item($node) {
    //Иконка и класс пункта дерева
    $item_icon = $this->get_tree_item_icon($node);
    $icon_open = $item_icon['icon_open'];
    $class = $item_icon['class'];

    // Добавление (только если есть родительское поле)
    if (isset($this->_options['parent_field'])) {
      $form_add = $this->get_add_form($node['nid']);
      $icon_add = $form_add['icon'];
      $form_add = $form_add['form'];
    }
    else {
      $icon_add = '';
      $form_add = '';
    }

    // Удаление
    $form_delete = $this->get_delete_form($node);
    $icon_delete = $form_delete['icon'];
    $form_delete = $form_delete['form'];

    //Копирование
    $form_copy = $this->get_copy_form($node['nid']);
    $icon_copy = $form_copy['icon'];
    $form_copy = $form_copy['form'];

    //Редактирование
    $form_edit = $this->get_edit_form($node['nid']);
    $icon_edit = $form_edit['icon'];
    $form_edit = $form_edit['form'];

    //Иконка с атрибутамми копирования, удаления и т.д.
    $icon_attr = '<div class="toggle_actions collapse" data-collapse="action-' . $node['nid'] . '"></div>
        <div class="actions_wrapper collapse"
        id = "action-' . $node['nid'] . '"
        style = "display: none;"
        > ' . $icon_add . $icon_delete . $icon_copy . $icon_edit . '</div>';

    // Суфикс
    $suffix = '';
    if (isset($this->_options['suffix']) && trim($this->_options['suffix'])){
        $suffix = '/'.trim($this->_options['suffix']);
    }

    //Есле передан URL для кликов по элемента дерева
    if (isset($this->_options['url']) && $this->_options['url']) {
      $name = l($node['title'], $this->_options['url'] . '/' . $node['nid'].$suffix, array(
        'attributes' => array('id' => 'item_href_' . $node['nid'])
      ));// Название-ссылка
    }
    else {
      //Даже если нет URL оформляем как ссылку. Это нужно для редактирования элементов
      //Не используем функция l() т.к. там значение # меняется на %23 (http://www.drupal.ru/node/52629)
      $name = '<a href="#" id="item_href_' . $node['nid'] . '">' . $node['title'] .$suffix. '</a>';
    }


    //Уровни элементов деревая для стилизации
    array_push($class, "level-" . $this->_options['level']);

    //Конечные результат для списка (если JSON или обычные списки дерева)
    if (isset($this->_options['json']) && ($this->_options['json'])) {
      $item = array(
        'name' => $node['title'],
        'nodeid' => $node['nid'],
      );
    }
    else {
      //Если нужна админка для дерева
      if (isset($this->_options['admin']) && $this->_options['admin']) {
        $forms_admin = $form_add . $form_delete . $form_copy . $form_edit;
      }
      else {
        $forms_admin = '';
        $icon_attr = '';
      }

      $item_options = array(
        'node' => $node,
        'name' => $name,
        'class' => $class,
        'icon_open' => $icon_open,
        'icon_attr' => $icon_attr,
        'forms' => $forms_admin
      );
      $item = $this->get_tree_item_html($item_options);
    }

    //Если строим раскрытое дерево
    if (isset($this->_options['parents']) && in_array($node['nid'], $this->_options['parents']) || (isset($this->_options['all_open']) && $this->_options['all_open'])) {
      $options = $this->_options;
      $options['root_id'] = $node['nid'];
      $tree_child = new $this->_options['class']($options);
      $tree_child_items = $tree_child->get_tree_items();
      if ($tree_child_items) {
        $item['children'] = $tree_child_items;

        //Защита для старой версии темы.
        if (isset($GLOBALS['base_theme_info'][0]->info['version']) && '7.x-3.0' != $GLOBALS['base_theme_info'][0]->info['version']) {
          $item['children']['id'] = "tree-" . $node['nid'];
          $item['children']['class'][] = "tree";
        }

        if (!isset($this->_options['json']) || !$this->_options['json']) {
          $item['class'][] = 'tree';
        }
      }
    }

    // Если дерево без админки, то фиксируем его ширину
    if (!isset($this->_options['admin']) || !$this->_options['admin']) {
      $item['class'][] = 'fixed';
    }

    return $item;
  }

  /**
   * Функция стилизации одного HTML-документа
   * @param $options = array(
   *      node        - Элемента дерева
   *      name        - Название элемента дерева
   *      class       - Класс будет добавлен ко всему элементу дерева
   *      icon_open   - Иконка открытия/закрытия элемента дерева
   *      icon_attr   - Набор иконок с элементами управления
   *      forms       - Формы добавления, удаления и копирования
   * )
   * @return array На выходе элемент массива items для формирования списка через функция item_list()
   */
  protected function get_tree_item_html($options = array()) {

    // Активный элемент дерева
    $data_class = '';
    if (isset($this->_options['target_id']) && $this->_options['target_id'] == $options['node']['nid']) {
      $data_class = 'active';
    }

    // Галочка
    if (global_tree_check_show()) {
      $class = '';
    }
    else {
      $class = 'hide';
    }
    $check = '<span class="label label-success label-check ' . $class . '">
                    <i class="fa fa-square-o item-check" data-id=' . $options['node']['nid'] . '></i>
              </span>';

    // Блок с actions
    $actions = '';
    if (isset($this->_options['admin']) && $this->_options['admin']) {
      $actions = "<div class='item_actions'>" . $options['icon_attr'] . "</div>";
    }

    //TODO: Подумать как это делается правильно. И может ли это что-то попортить.
    $this->current_id = $options['node']['nid'];
    $this->current_label = $options['node']['title'];
    $this->current_child_count = $options['node']['child_count'];


    $data = "
        <div class='tree_node tree-element-data'>
          <div class='item_indent'></div>
          <div class='item_wrapper'>
            <div class='tree_item_content'>
              <div class='item_toggled'>" . $options['icon_open'] . "</div>
              <div class='item_label'>" . $this->get_tree_item_value($options['name'] . $this->global_tree_get_attributes_count($options['node']['nid'])) . "</div>
            </div>
            <div class='tree_item_attributes' id='item-attributes-" . $options['node']['nid'] . "'>
              $check
            </div>
          </div>
          $actions
        </div>
        <div class='item_forms'>" . $options['forms'] . "</div>";

    return array(
      'data' => '<div id="data-' . $options['node']['nid'] . '" data-nid="'.$options['node']['nid'].'" class="tree_item ' . $data_class . '">' . $data . '</div>',
      'id' => 'i-' . $options['node']['nid'],
      'class' => $options['class'],
    );
  }

  /**
   * Формируем заголовок, который будет выводиться в строке дерева
   * @param $value Значение для вывода
   * @return mixed HTML блок или строка
   */
  public function get_tree_item_value($value) {
    return $value;

  }

  /**
   * Функция для получения или обновления при добавлении атрибутов элемента дерева
   * @return string
   */
  public function get_tree_item_attr() {
    return '';
  }


  /**
   * Элемент дерева с исходными данными для отображения
   * На выходе: Идендификатор, Название, кол-во детей, и ID детей, если они есть
   */
  protected function get_tree_item_res() {
    $node_wrapper = entity_metadata_wrapper($this->_options['entity_type'], $this->_options['root_id']);

    //Если есть поле родителя, то запрашиваем детей, если его нет, то и запрашивать не нужно.
    if (isset($this->_options['parent_field'])) {
      $data = $this->get_tree_data();
      if (isset($data)) {
        $childs = array_keys($data); // Только ID детей
      }
      else {
        $childs = array();
      }
    }
    else {
      $childs = array();
    }

    return array(
      'nid' => $node_wrapper->getIdentifier(),
      'title' => $node_wrapper->label(),
      'child_count' => count($childs),
      'childs' => $childs
    );
  }


  /**
   * Подсчёт кол-ва атрибутов, которые относятся к данному элементу дерева
   * @param $id ID элемента дерева. Остальные параметры берутся из глобальных настроек дерева
   * Это: тип атрибутов и поле по которому они связаны с элементом
   * @return string
   */
  protected function global_tree_get_attributes_count($id) {
    $str_count = '';
    if (isset($this->_options['attributes_field'])) {
      $query = db_select('field_data_' . $this->_options['attributes_field'], 't');
      $query->condition('t.' . $this->_options['attributes_field'] . '_' . $this->_options['entity_type_id'], $id);
      $query->condition('t.bundle', $this->_options['attributes_bundle']);
      //$query->addTag('node_access')
      $count = $query->countQuery()->execute()->fetchField();
      if ($count) {
        $str_count = " ($count) ";
      }
    }
    return $str_count;
  }


  /**
   * Внутрення функция формирования иконки и класса у одног элемента списка в дереве
   * @param $node Нода дерева
   * @return array На выходе массив из иконки-ссылк и класс для элемента дерева
   */
  public function get_tree_item_icon($node) {
    $query = $this->_options;
    $query['root_id'] = $node['nid'];

    $icon_open = ''; //Иконка отображения пункта дерева
    $class = array(); //Класс для всего пункта дерева
    // Это элемент раскрытого дерева
    if (isset($this->_options['parents']) && in_array($node['nid'], $this->_options['parents']) || (isset($this->_options['all_open']) && $this->_options['all_open'])) {
      if ($node['child_count'] == 0) {
        $icon_open = l('<div class="i-open i-empty"></div><span class="fa fa-folder-open-o"></span>', '', array(
          'attributes' => array('class' => array('icon-open')),
          'html' => TRUE
        ));
        $class = array('load', 'empty', 'opened');
      }
      else {
        $icon_open = l('<div class="i-open i-full"></div><span class="fa fa-folder-open"></span>', '', array(
          'attributes' => array('class' => array('icon-open')),
          'html' => TRUE
        ));
        $class = array('load', 'opened');
      }
    }
    elseif ((isset($node['child_count']) && $node['child_count'] == 0) || (!isset($this->_options['parent_field']) && ($this->_options['entity_type'] == 'node'))) {//Нет детей
      $icon_open = l('<div class="i-close i-empty"></div><span class="fa fa-folder-o"></span>', '', array(
        'attributes' => array('class' => array('icon-open')),
        'html' => TRUE
      ));
      $class = array('load', 'empty');
    }
    else {
      $query['f'] = 'global_tree_ajax';
      $icon_open = l('<div class="i-close i-full"></div><span class="fa fa-folder"></span>', 'l/ajax', array(
        'query' => $query,
        'html' => TRUE,
        'attributes' => array('class' => array('use-ajax', 'icon-open'))
      ));
    }

    return array('icon_open' => $icon_open, 'class' => $class);
  }


  /**
   * Кнопка и форма добавления элементов в дерево
   * @param $nid ID node
   * @return array Массив из кнопки и формы (collapse-связь)
   */
  protected function get_add_form($nid) {
    $icon_add = '';
    $form_add = '';
    if (isset($this->_options['admin']) && ((!is_array($this->_options['admin']) && $this->_options['admin']) || (is_array($this->_options['admin'])))) {
      if (isset($nid) && $nid != $this->_options['root']) {
        $title = '';
      }
      else {
        if (isset($this->_options['top']['show_title']) && $this->_options['top']['show_title'] == FALSE) {
          $title = '';
        }
        else {
          $title = 'Добавить';
        }
      }
      
      $icon_add = l($title, '', array(
        'attributes' => array(
          'class' => array('tree_btn', 'btn_add', 'collapse'),
          'title' => 'Добавить',
          'data-collapse' => 'div-form-add-' . $nid,
        ),
        'html' => TRUE,
      ));

      $options = $this->_options;
      $options['root_id'] = $nid;
      $form_state['values'] = array();
      $form_state['input'] = array();
      $form_state['build_info']['args'] = array($options);

      //Построение самой формы добавления с нужными полями
      $form = drupal_build_form($this->_options['function_form_add'], $form_state);
      $form_add = '<div class="collapse form-add" id="div-form-add-' . $nid . '">
                           <div class="well">
                               ' . drupal_render($form) . '
                           </div>
                       </div>';
    }
    return array('icon' => $icon_add, 'form' => $form_add);
  }


  /**
   * Форма удаления элементов дерева
   * @param $nid ID объекта удаления
   * @return array Массив-связка: иконка удаления и форма удаления (collapse-cвязь)
   */
  protected function get_delete_form($node) {
    $nid = $node['nid'];
    $icon_delete = '';
    $form_delete = '';
    if (isset($this->_options['admin']) && ((!is_array($this->_options['admin']) && $this->_options['admin']) || (is_array($this->_options['admin']) && in_array("delete", $this->_options['admin'])))) {
      $query = $this->_options;
      $query['root_id'] = $nid;
      $query['f'] = 'global_tree_delete';
      $icon_delete = l('', '', array(
        'attributes' => array(
          'class' => array('tree_btn', 'btn_delete', 'collapse'),
          'title' => 'Удалить',
          'data-collapse' => 'form-delete-' . $nid,
        ),
        'html' => TRUE,
      ));
      $form_delete = '<div class="collapse form-copy" id="form-delete-' . $nid . '">
							<div class="well">
								Вы действительно хотите удалить данный объект?
								<button type="button" class="btn btn-default btn-xs btn-close">Отмена</button>
								' . l('Удалить', 'l/ajax', array(
          'query' => $query,
          'html' => TRUE,
          'attributes' => array(
            'class' => array('use-ajax', 'btn', 'btn-danger', 'btn-xs'),
          )
        )) . '
							</div>
						</div>';
    }
    return array('icon' => $icon_delete, 'form' => $form_delete);
  }


  /**
   * Форма копирования элементов дерева
   * @param $nid ID обекта копирования
   * @return array Массив из иконки и формы копирования (collapse - связь)
   */
  protected function get_copy_form($nid) {
    $icon_copy = '';
    $form_copy = '';
    $form_id = 'div-form-copy-' . $nid;
    if (isset($this->_options['admin']) && ((!is_array($this->_options['admin']) && $this->_options['admin']) || (is_array($this->_options['admin']) && in_array("copy", $this->_options['admin'])))) {
      $icon_copy = l('', '', array(
        'attributes' => array(
          'class' => array('tree_btn', 'btn_copy', 'collapse'),
          'title' => 'Копировать',
          'data-collapse' => 'div-form-copy-' . $nid,
          'data-form-id' => $form_id,
        ),
        'html' => TRUE,
      ));

      $options = $this->_options;
      $options['action'] = 'copy';
      $options['id'] = $nid;
      $options['root_id'] = $nid;
      $form_state['values'] = array();
      $form_state['input'] = array();
      $form_state['build_info']['args'] = array($options);

      //Построение самой формы добавления с нужными полями
      $form = drupal_build_form($this->_options['function_form_add'], $form_state);
      $form_copy = '<div class="collapse form-copy" id="' . $form_id . '"><div class="well">' . drupal_render($form) . '</div></div>';
    }
    return array('icon' => $icon_copy, 'form' => $form_copy);
  }


  /**
   * Форма для редактирования объекта дерева
   * @param $nid
   * @return array
   */
  protected function get_edit_form($nid) {
    $icon_add = '';
    $form_add = '';
    $form_id = 'div-form-edit-' . $nid;
    if (isset($this->_options['admin']) && ((!is_array($this->_options['admin']) && $this->_options['admin']) || (is_array($this->_options['admin']) && in_array("edit", $this->_options['admin'])))) {
      $icon_add = l('', '', array(
        'attributes' => array(
          'class' => array('tree_btn', 'btn_edit', 'collapse'),
          'title' => 'Редактировать',
          'data-collapse' => 'div-form-edit-' . $nid,
          'data-form-id' => $form_id,
        ),
        'html' => TRUE,
      ));

      $options = $this->_options;
      $options['id'] = $nid;
      $options['root_id'] = $nid;
      $form_state['values'] = array();
      $form_state['input'] = array();
      $form_state['build_info']['args'] = array($options);

      //Построение самой формы добавления с нужными полями
      $form = drupal_build_form($this->_options['function_form_add'], $form_state);
      $form_add = '<div class="collapse form-add" id="' . $form_id . '">
                           <div class="well">
                               ' . drupal_render($form) . '
                           </div>
                       </div>';
    }
    return array('icon' => $icon_add, 'form' => $form_add);
  }


  /**
   * Метод по формированию массива с исходными данными для построения дерева
   * @return array (nid, title, child_count)
   */
  public function get_tree_res() {
    $result = $this->get_tree_data();
    $res = array();
    if (isset($result)) {
      $options = $this->_options; // Сохраняем первоначальные опции

      foreach ($result as $id => $title) {
        //Новые опции для получения дочерних элементов 1-ого уровня
        $this->_options['root_id'] = $id;
        $this->_options['target_id'] = NULL;
        $res[] = $this->get_tree_item_res();
      }
      $this->_options = $options; // Возвращаем первоначальные опции для дерева
    }
    $this->_childs_count = count($res); // Количество детей для стилизации
    return $res;
  }


  /**
   * Получаем данные из БД
   * @return mixed
   * @throws \EntityFieldQueryException
   */
  protected function get_tree_data() {

    $query = db_select($this->_options['entity_type'], 'n');
    $query->fields('n', array('nid', 'title'));
    $query->condition('n.type', $this->_options['bundle']);
    $query->addTag('node_access');

    if ($this->_options['entity_type'] == 'node') {
      $query->condition('n.status', NODE_PUBLISHED); // Только опубликованные материалы

      // Сортировка
      // TODO: Сортировку по отдельному полю, нужно отдельно проработать
      if (isset($this->_options['order']['field'])) {

        $order_type = 'ASC';
        if (isset($this->_options['order']['type'])) {
          $order_type = $this->_options['order']['type'];
        }

        $query->leftJoin('field_data_' . $this->_options['order']['field'], 'weight', 'weight.entity_id = n.nid');
        $query->orderBy("weight." . $this->_options['order']['field'] . "_value", $order_type);
      }
      else {
        //$query->propertyOrderBy('title', 'ASC');
        $query->orderBy('n.title', 'ASC');
      }

      // Связь по родительскому полю
      if (isset($this->_options['parent_field']) && $this->_options['parent_field']) {
        if (isset($this->_options['root_id']) && $this->_options['root_id']) {
          $query->leftJoin('field_data_' . $this->_options['parent_field'], 'p', 'p.entity_id = n.nid');
          $query->condition('p.' . $this->_options['parent_field'] . '_' . $this->_options['entity_type_id'], $this->_options['root_id']);
          $query->condition("p.bundle", $this->_options['bundle']);
        }
        else {
          // Или корневые элементы без родителей
          $query->leftJoin('field_data_' . $this->_options['parent_field'], 'p', "n.nid = p.entity_id");
          $query->isNull('p.' . $this->_options['parent_field'] . '_' . $this->_options['entity_type_id']);
        }
      }
    }

    // Для терминов таксономии
    elseif ($this->_options['entity_type'] == 'taxonomy_term') {
      // В таксономии делаем сортировку по весу, которая выставляется в админке
      // $query->propertyOrderBy('title', 'ASC');
      $query->propertyOrderBy('name', 'ASC');
      $query->addMetaData('parent', $this->_options['root_id']);
      $query->addTag('taxonomy_parent');
    }

    return $query->execute()->fetchAllKeyed();
  }


  /**
   * Функция для формирования массива из предков всех уровней для данного элемента
   * @param $nid ID ноды объекта для которого ищем его предка
   * Используем хранимку Новикова Евгения для получения ID предков всех уровней
   * @return array|null Массив из предков всех уровней для данного элемента
   * Результирующий массив вида: 1 - XXXXX, 2 - YYYYY и т.д.
   */
  public function get_parents($id) {
    if ($id) {
      return global_tree_get_all_parents($id, $this->_options['parent_field']);
    }
    else {
      return NULL;
    }
  }


  /**
   * Статический метод добавления новых объектов в дерево на основании данных из формы (в форме пока только название)
   * @param $form_state
   * @throws \EntityFieldQueryException
   */
  public static function add($options) {
    global $user;

    if(isset($options['entity_type']) && $options['entity_type'] == 'node' && node_access('create', $options['bundle']))
    {
      $entity = entity_create($options['entity_type'], array('type' => $options['bundle']));
      $wrapper = entity_metadata_wrapper($options['entity_type'], $entity);
      $wrapper->author->set($user->uid);
      $wrapper->status->set(1);
      $wrapper->title->set($options['values']['title']);

      // Если не корневой элемент
      if ($options['root_id']) {
        $field_object = field_info_field($options['parent_field']);
        $parent_to_set = array($options['root_id']);
        if ($field_object['cardinality'] == 1) {
          $parent_to_set = $parent_to_set[0];
        }
        $wrapper->$options['parent_field']->set($parent_to_set);
      }

      // Если есть поле вес или аналогичное, то добавляем элемент в конец списка
      if (isset($options['order']['field'])) {
        $max_weight = $options['class']::get_weight_max($options);
        $wrapper->{$options['order']['field']}->set($max_weight + 1);
      }

      $wrapper->save();
      return $wrapper->getIdentifier();
    }
  }


  /**
   * Функция клонирования элемента дерева
   * @param $nid ID объекта, который необходимо клонировать
   * @return array Результат клонирвоания массив(ID, tittle, кол-во детей)
   * @throws \EntityFieldQueryException
   * @throws \Exception
   */
  public static function copy($options, $new_parent_id = NULL) {
    $nid = $options['root_id'];
    module_load_include('inc', 'entity', 'includes/entity.ui');
    $node_donor = entity_load_unchanged($options['entity_type'], $nid);
    $node_clone = entity_ui_clone_entity($options['entity_type'], $node_donor);
    unset($node_clone->nid);
    unset($node_clone->vid);

    // Если есть новое имя для элемента, оно не пустое, и это не дети, то запоминаем это имя
    if (isset($options['values']['title']) && $options['values']['title'] != '' && !isset($new_parent_id)) {
      $node_clone->title = $options['values']['title'];
    }

    //Если клонируются дети, то у них привязка к новому клонированному родителю
    if ($new_parent_id) {
      $node_clone->{$options['parent_field']}['und'][0]['nid'] = $new_parent_id; // Родитель
    }
    node_save($node_clone); //Сохраняем клон

    //Клонируем детей
    $tree_new = new $options['class']($options);
    $tree_new_res = $tree_new->get_tree_res();
    if ($tree_new_res) {
      foreach ($tree_new_res as $child) {
        $options['root_id'] = $child['nid'];
        $options['class']::copy($options, $node_clone->nid);
      }
    }

    //Сам клонируемый объект
    $options['root_id'] = $node_clone->nid;
    $clone = new $options['class']($options);
    return $clone->get_tree_item_res();
  }


  /**
   * Функция рекурсивного удаления объекта и всех его потомков
   * @param $options массив опций из GlobalTree, где root_id это ID элемента, который необходимо удалить
   * и который является родителем по отношению к другим элементам при удалении
   */
  public static function delete($options) {

    $el_id = $options['root_id']; //Сохраняем id элемента который нужно удалить.

    // Сначала удаляем потомков, если это задано в опциях и они существуют
    // или если в опциях ничего не задано.
    // Или если это обычнй линейный список без вложенности
    if (isset($options['parent_field']) && (!isset($options['delete']) || (isset($options['delete']['recursive']) && $options['delete']['recursive']))) {
      $childs = $options['class']::get_childs($options);
      if (isset($childs)) {
        foreach ($childs as $nid => $child) {
          $options['root_id'] = $nid;
          $options['class']::delete($options);
        }
      }
    }

    $options['root_id'] = $el_id; //Восстанавливаем id элемента который нужно удалить

    //В конце удаляем сам объект
    if (isset($options['delete']['function'])) {
      if (function_exists($options['delete']['function'])) {
        $function_delete = $options['delete']['function'];
        $function_delete($options['root_id']);//Вызывается "удалённая" функция удаления объекта
      }
      else {
        $n = node_load($options['root_id']);
        if(node_access('delete', $n)) node_delete($options['root_id']);
      }
    }
    else {
      $n = node_load($options['root_id']);
      if(node_access('delete', $n)) node_delete($options['root_id']);
    }
  }


  /**
   * @param $options
   * @return mixed
   * @throws \EntityFieldQueryException
   */
  public static function edit($options) {
    $wrapper = entity_metadata_wrapper($options['entity_type'], $options['id']);
    $wrapper->title->set($options['values']['title']);
    $wrapper->save();
    return $wrapper->getIdentifier();
  }


  /**
   * Функция получения непосредсвтвеных потомков объекта по его ID
   * @param $options атрибуты из GlobalTree, для нахождения потомков по связанным полям
   * @return array|bool массив детей, либо FALSE если непосредственные потомки отсутствуют
   */
  public static function get_childs($options) {
    if ($options['entity_type'] == 'node') {
      $query = db_select($options['entity_type'], 'n');
      $query->fields('n', array('nid', 'title'));
      $query->addTag('node_access');
      $query->condition('n.type', $options['bundle']);
      $query->innerJoin('field_data_' . $options['parent_field'], 'c', 'c.entity_id = n.nid');
      $query->condition('c.' . $options['parent_field'] . '_' . $options['entity_type_id'], $options['root_id']);
      $childs = $query->execute()->fetchAllKeyed();
      if (count($childs) > 0) {
        return $childs;
      }
      else {
        return FALSE;
      }
    }
    // Это для терминов таксономии
    elseif ($options['entity_type'] == 'taxonomy_term') {
      $count = db_select('taxonomy_term_hierarchy', 'h')->condition('h.parent', $options['root_id'])->countQuery()->execute()->fetchField();
      return $count;
    }
    else {
      return FALSE;
    }
  }

  /**
   * Функция отображения дерева в JSON формате
   * @return string json-строка
   */
  public function get_tree_json() {
    $this->_options['all_open'] = TRUE;
    $this->_options['json'] = TRUE;
    $json = array();
    if (isset($this->_options['root_id']) && $this->_options['root_id'] > 0) {
      $entity = entity_metadata_wrapper('node', $this->_options['root_id']);
      $json['name'] = $entity->label();
      $json['nodeid'] = $entity->getIdentifier();
      $json['children'] = $this->get_tree_items();
    }
    else {
      $json = $this->get_tree_items();
    }
    return json_encode($json);
  }


  /**
   * HTML-разметка строки поиска
   * @return string на выходе строка поиска
   */
  public function get_search_html() {
    //Если есть что-то в строке поиска
    if (isset($this->_options['target_id']) && $this->_options['target_id'] > 0) {
      $entity = entity_metadata_wrapper($this->_options['entity_type'], $this->_options['target_id']);
      $value = $entity->label();
    }
    else {
      $value = '';
    }
    $data_url = isset($this->_options['url']) ? $this->_options['url'] : '';
    return '<div class="top-search">
<div class="input-group">
            <input id="autocomplete" class="form-control" value="' . $value . '" placeholder="Поиск">
            <span class="input-group-addon" id="basic-addon2"><i class="fa fa-search"> </i> Искать элементы</span>
</div>
    </div>
    <div class="top-search-list" data-url="' . $data_url . '"
          data-options="entity_type=' . $this->_options['entity_type'] . '&bundle=' . $this->_options['bundle'] . '">
        </div>
    ';
  }


  /**
   * Получение максимального занятого поля вес
   * @param $options все параметры дерева для стат.метода
   */
  public static function get_weight_max($options) {
    $query = db_select($options['entity_type'], 'n');
    $query->fields('weight', array($options['order']['field'] . "_value"));
    $query->condition('n.status', NODE_PUBLISHED);
    $query->condition('n.type', $options['bundle']);
    $query->addTag('node_access');
    $query->range(0, 1);
    $query->leftJoin('field_data_' . $options['order']['field'], 'weight', 'weight.entity_id = n.nid');
    $query->orderBy("weight." . $options['order']['field'] . "_value", 'DESC');
    $value = $query->execute()->fetchField();

    // Если есть уже существующие, то возвращаем, если нет, то начинаем новую нумерацию
    if ($value) {
      return $value;
    }
    else {
      return 0;
    }
  }

  public function init_static_content() {
    drupal_add_css(drupal_get_path('module',
        'global_tree') . '/css/global_tree.css');

    //Сортировка
    drupal_add_library('system', 'ui.sortable');
    drupal_add_library('system', 'ui.draggable');
    drupal_add_js(array(
      'module_path' => drupal_get_path('module', 'global_tree')
    ), 'setting');
    drupal_add_js(drupal_get_path('module',
        'global_tree') . '/js/jquery.mjs.nestedSortable.js');
    drupal_add_js(drupal_get_path('module',
        'global_tree') . '/js/global_tree.js');
  }
}

