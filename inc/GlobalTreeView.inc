<?php


/**************************************************************************************
 * Универсальный класс по представлению взаимосвязанных данных в виде дерева
 * date: 18.02.2015
 * description: В дочерних классах необходимо переопределять функции
 *    get_tree_res: если выборка будет проходить по нескольким нетипичным полям
 *    global_tree_query_has_not_parent_alter: дополнительный JOIN для корневых элементов
 **************************************************************************************/
class GlobalTreeView {

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

      //TODO Получить тех кого надо раскрывать для сложного дерева. Если это возможно.
      //$this->_options['parents'] = $this->get_parents($options['target_id']);

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
  }


  /**
   * Фуникция отрисовки html-дерева
   * @return string
   * @throws \Exception
   */
  public function get_tree_html() {
    drupal_add_css(drupal_get_path('module', 'global_tree') . '/css/global_tree.css');
    drupal_add_js(drupal_get_path('module', 'global_tree') . '/js/global_tree.js');

    drupal_add_js(array('module_path' => drupal_get_path('module', 'global_tree')), 'setting');

    $items = array();

    //Если многокорневое дерево
    if (isset($this->_options['root_id']))
    {
      if(!is_array($this->_options['root_id'])) $this->_options['root_id'] = array($this->_options['root_id']);
      $root_ids = $this->_options['root_id'];

      foreach ($root_ids as $root_id)
      {
        $this->_options['root_id'] = $root_id;

        if (isset($this->_options['root_show']) && $this->_options['root_show'])
        {
          $entity = entity_metadata_wrapper($this->_options['entity_type'], $this->_options['root_id']);
          $node['title'] = $entity->label();
          $node['nid'] = $entity->getIdentifier();

          //Есть дети у многокорневого дерева или нет
          $options = $this->_options;
          $options['root_id'] = $entity->getIdentifier();
          // Для проверки тестового комита
          $childs = $options['class']::get_childs($options);
          if ($childs) {
            $node['child_count'] = 1;
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
    //$tree_html = str_replace('<div class="item-list">', "", $tree_html);
    //$tree_html = str_replace('</ul></div>', "</ul>", $tree_html);

    // Если это пустое дерево, т.е. не было связки </ul></div>
    if ($tree_html == '</div>') {
      $tree_html = '';
    }

    // Добавляем кнопки управления деревом, если дерево не пустое
    if ($tree_html == '')
    {
      $tree_html = '<div id="tree-' . $root_id . '"><div class="alert alert-info" role="alert">Нет элементов для отображения</div></div>';
    }

    if (!empty($this->_options['json'])){
    self::provide_json();
   }
    return $tree_html;
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
  protected function get_tree_items()
  {
    $res = $this->get_tree_res();

    $items = array();
    foreach ($res as $node) $items[] = $this->get_tree_item($node);

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


    //Есле передан URL для кликов по элемента дерева
    if (isset($this->_options['url']) && $this->_options['url'])
    {
      // Название-ссылка
      $name = l($node['title'], $this->_options['url'] . '/' . $node['nid'], array('attributes' => array('id' => 'item_href_' . $node['nid'])));
    }
    else
    {
      //Даже если нет URL оформляем как ссылку. Это нужно для редактирования элементов
      //Не используем функция l() т.к. там значение # меняется на %23 (http://www.drupal.ru/node/52629)
      $name = '<a href="#" id="item_href_' . $node['nid'] . '">' . $node['title'] . '</a>';
    }

    //Уровни элементов деревая для стилизации
    array_push($class, "level-" . $this->_options['level']);

    //Конечные результат для списка (если JSON или обычные списки дерева)
    if (isset($this->_options['json']) && ($this->_options['json']))
    {
      $item = array(
        'name' => $node['title'],
        'nodeid' => $node['nid'],
      );
    }
    else
    {
      $forms_admin = '';
      $icon_attr = '';

      $e = entity_metadata_wrapper("node", $node['nid']);
      $current_bundle = $e->getBundle();
      
      $ajax_callback = [];
      if (isset($this->_options['bundle_settings'][$current_bundle]['ajax_callback'])) $ajax_callback = $this->_options['bundle_settings'][$current_bundle]['ajax_callback'];
      
      
      $item_options = array(
        'node' => $node,
        'name' => $name,
        'class' => $class,
        'icon_open' => $icon_open,
        'icon_attr' => $icon_attr,
        'forms' => $forms_admin,
        'ajax_callback' => $ajax_callback
      );
      $item = $this->get_tree_item_html($item_options);
    }

    //Если строим раскрытое дерево
    if (isset($this->_options['parents']) && in_array($node['nid'], $this->_options['parents']) || (isset($this->_options['all_open']) && $this->_options['all_open']))
    {
      $options = $this->_options;
      $options['root_id'] = $node['nid'];
      $tree_child = new $this->_options['class']($options);
      $tree_child_items = $tree_child->get_tree_items();

      if ($tree_child_items)
      {
        $item['children'] = $tree_child_items;
        if (!isset($this->_options['json']) || !$this->_options['json']) $item['class'][] = 'tree';
      }
    }

    return $item;
  }

  /**
   * Функция стилизации одного HTML-документа
   * @param $options = array(
   *      node          - Элемента дерева
   *      name          - Название элемента дерева
   *      class         - Класс будет добавлен ко всему элементу дерева
   *      icon_open     - Иконка открытия/закрытия элемента дерева
   *      icon_attr     - Набор иконок с элементами управления
   *      forms         - Формы добавления, удаления и копирования
   *      ajax_callback - Массив: адрес callback-а, который возвращает AJAX-команды, и (опционально) допонительные uri
   * )
   * @return array На выходе элемент массива items для формирования списка через функция item_list()
   */
  protected function get_tree_item_html($options = array())
  {
    $e = entity_metadata_wrapper("node", $options['node']['nid']);
    $type = $e->getBundle();


    //Расссчет числа объектов различных типов.
    if(isset($this->_options['count_object_settings']))
    {
      foreach($this->_options['count_object_settings'] as $key => $value)
      {
        //Рассчет числа объектов типа $key.

        //$count_objects = $this->get_count_objects($options['node']['nid'], $key, $value);
        //$count_objects = 0;


        if($type == $value['type'])
        {

          $query = db_select("node", 'n');
          $query->fields('n', array('nid', 'title'));
          $query->condition('n.type', $key);
          $query->leftJoin('field_data_' . $value['field'], 'p','p.entity_id = n.nid');
          $query->condition('p.' . $value['field'] . '_target_id',$options['node']['nid']);
          $query->condition("p.bundle", $key);

          //$count_self_objects = $query->countQuery()->execute()->fetchField();
        }
        //1 нужно посмотреть с каким контент тайпом он связан.

        //2 Если Текущий контент тайп равен тому по которому связаны объекты, то получаем дочки через процедуру и считаем объекты.

        //3 Если текущий контент тайп выше того, по которому связаны объекты, нужно получить всех детей и вызвать эту же функцию.

        //4. Если текущий контент тайп ниже того к которому привязаны объекты то по идее ничего делать не нужно
      }
    }




    // Активный элемент дерева
    $data_class = '';
    if (isset($this->_options['target_id']) && $this->_options['target_id'] == $options['node']['nid']) $data_class = 'active';


    //AJAX-callback
    if (!empty($options['ajax_callback']))
    {
      $ajax_url = null;
      if (!empty($options['ajax_callback']['link']))
      {
        if (!empty($options['ajax_callback']['json']) and $options['ajax_callback']['json'] == true) {
          $options['ajax_callback']['link'] = 'globaltreejson/' . $options['ajax_callback']['link'];
        }
        $ajax_url .= "<div id='global_load_ajax_wrapper_".$options['node']['nid']."_0' class='global_load_ajax_wrapper' data-url='" . $options['ajax_callback']['link'] . "/" . $options['node']['nid'];

        if (isset($options['ajax_callback']['uri_params']))
        {
          $ajax_url .= '/' . $options['ajax_callback']['uri_params'];
        }
        $ajax_url .= "'> </div>";
      }
      else
      {
        foreach ($options['ajax_callback'] as $i => $ajax)
        {
          if (!empty($ajax['json']) and $ajax['json'] == true) {
            $ajax['link'] = 'globaltreejson/' . $ajax['link'];
          }
          $ajax_url .= "<div id='global_load_ajax_wrapper_".$options['node']['nid']."_" .$i. "' class='global_load_ajax_wrapper' data-url='" . $ajax['link'] . "/" . $options['node']['nid'];
          if (isset($ajax['uri_params'])) $ajax_url .= '/' . $ajax['uri_params'];
          $ajax_url .= "'> </div>";
        }
      }
    }
    else $ajax_url = '';

    $data = "
        <div class='tree_node tree-element-data'>
          <div class='item_indent'></div>
          <div class='item_wrapper'>
            <div class='tree_item_content'>
              <div class='item_toggled'>" . $options['icon_open'] . "</div>
              <div class='item_label'>"
                . $this->get_tree_item_value($options['name'], $options['node']['nid'], $options['node']['title'], $options['node']['child_count']) .
                "<div class='tree_item_attributes' id='item-attributes-" . $options['node']['nid'] . "'>
                $ajax_url
                </div>
              </div>
            </div>
          </div>
        </div>";

    return array(
      'data' => '<div id="data-' . $options['node']['nid'] . '" data-nid="'.$options['node']['nid'].'" class="tree_item ' . $data_class . '">' . $data . '</div>',
      'id' => 'i-' . $options['node']['nid'],
      'class' => $options['class'],
    );
  }

  /*
  protected function get_count_objects($id, $object_bundle, $object_settings)
  {
    $count = 0;

    $e = entity_metadata_wrapper("node", $id);
    $current_bundle = $e->getBundle();

    if($current_bundle == $object_settings['type'])
    {
      //Получить список всех детей
      $childs = global_tree_get_all_childs($id, $object_settings['field']);
      $childs = array_filter($childs);
      array_unshift($childs, $id);

      $query = db_select("node", 'n');
      $query->fields('n', array('nid', 'title'));
      $query->condition('n.type', $object_bundle);
      $query->leftJoin('field_data_' . $object_settings['field'], 'p','p.entity_id = n.nid');
      $query->condition('p.' . $object_settings['field'] . '_target_id',$id);
      $query->condition("p.bundle", $object_bundle);
      $count += $query->countQuery()->execute()->fetchField();

    }
    else
    {
      //Получить всех детей для текущего элемента.

      //Собрать вниз по дереву ид такого же контент тайпа
      $ids = global_tree_get_all_childs($id, $object_settings['field']);
      $ids = array_filter($ids);
      array_unshift($ids, $id);
      foreach($this->_options['bundle_settings'][$current_bundle]['childs'] as $item)
      {
        //Получить всех детей по списку собранных ид
        $query = db_select("node", 'n');
        $query->fields('n', array('nid', 'title'));
        $query->condition('n.type', $item['bundle']);
        $query->leftJoin('field_data_' . $item['field'], 'p','p.entity_id = n.nid');
        $query->condition('p.' . $item['field'] . '_target_id',$ids, "IN");
        $query->condition("p.bundle", $item['bundle']);
        $child_ids = $query->execute()->fetchCol();


        //$child_ids = db_select("field_data_".$item['field'], "alias")->fields("alias", array("entity_id"))->condition("alias.bundle", $item['bundle'])->condition("alias.".$item['field']."_target_id", $ids, "IN")->execute()->fetchCol();
        foreach($child_ids as $child_id)
        {
          $count += $this->get_count_objects($child_id, $object_bundle, $object_settings);
        }
      }
    }
    return $count;
  }
  */


  /**
   * Элемент дерева с исходными данными для отображения
   * На выходе: Идендификатор, Название, кол-во детей, и ID детей, если они есть
   */
  protected function get_tree_item_res()
  {
    $res = array();
    $node_wrapper = entity_metadata_wrapper($this->_options['entity_type'], $this->_options['root_id']);

    $is_next = FALSE;
    $options = $this->_options;

    if(isset($this->_options['root_id']))
    {
      $e = entity_metadata_wrapper($options['entity_type'], $options['root_id']);
      $current_bundle = $e->getBundle();
    }
    else
    {
      $current_bundle = array_keys($options['bundle_settings']);
      $current_bundle = current($current_bundle);
    }

    foreach($options['bundle_settings'] as $bundle => $data)
    {
      if($bundle == $current_bundle || $is_next == TRUE)
      {
        foreach($data['childs'] as $child)
        {
          $this->_options['parent_field'] = $child['field'];
          $this->_options['bundle'] = $child['bundle'];

          $result = $this->get_tree_data();
          if(isset($result) && !empty($result))
          {
            $options = $this->_options; // Сохраняем первоначальные опции
            foreach ($result as $id => $title) $res[] = $id;

            $this->_options = $options; // Возвращаем первоначальные опции для дерева
          }
        }
        $is_next = TRUE;
      }

      if($is_next || !isset($this->_options['root_id'])) break;
      //else $is_next = TRUE;
    }

    return array(
      'nid' => $node_wrapper->getIdentifier(),
      'title' => $node_wrapper->label(),
      'child_count' => count($res),
      'childs' => $res
    );
  }

  /**
   * Внутрення функция формирования иконки и класса у одног элемента списка в дереве
   * @param $node Нода дерева
   * @return array На выходе массив из иконки-ссылк и класс для элемента дерева
   */
  public function get_tree_item_icon($node)
  {
    $query = $this->_options;
    $query['root_id'] = $node['nid'];

    //Класс для всего пункта дерева
    $class = array();

    // Это элемент раскрытого дерева
    if(isset($this->_options['parents']) && in_array($node['nid'], $this->_options['parents']) || (isset($this->_options['all_open']) && $this->_options['all_open']))
    {
      if ($node['child_count'] == 0)
      {
        $icon_open = l(
          '<div class="i-open i-empty"></div><span class="fa fa-folder-open-o"></span>',
          '',
          array(
            'attributes' => array('class' => array('icon-open')),
            'html' => TRUE
          )
        );
        $class = array('load', 'empty', 'opened');
      }
      else
      {
        $icon_open = l(
          '<div class="i-open i-full"></div><span class="fa fa-folder-open"></span>',
          '',
          array(
            'attributes' => array('class' => array('icon-open')),
            'html' => TRUE
          )
        );
        $class = array('load', 'opened');
      }
    }
    elseif((isset($node['child_count']) && $node['child_count'] == 0))
    {
      //Нет детей
      $icon_open = l(
        '<div class="i-close i-empty"></div><span class="fa fa-folder-o"></span>',
        '',
        array(
          'attributes' => array('class' => array('icon-open')),
          'html' => TRUE
        )
      );
      $class = array('load', 'empty');
    }
    else
    {
      //Удаляем root_show и root чтобы ссылка для получения дерева для детей построилась корректно
      unset($query['root_show']);
      unset($query['root']);

      $query['f'] = 'global_tree_ajax';
      $icon_open = l(
        '<div class="i-close i-full"></div><span class="fa fa-folder"></span>',
        'l/ajax',
        array(
          'query' => $query,
          'html' => TRUE,
          'attributes' => array('class' => array('use-ajax', 'icon-open'))
        )
      );
    }

    return array('icon_open' => $icon_open, 'class' => $class);
  }


  /**
   * Метод по формированию массива с исходными данными для построения дерева
   * @return array (nid, title, child_count)
   */
  public function get_tree_res()
  {
    $current_res = array(); // Текущий уровень
    $all_childs = array(); //Все дети

    //Нужно из текущего убрать всех детей

    $is_next = FALSE;
    $options = $this->_options;

    if(isset($this->_options['root_id']))
    {
      $e = entity_metadata_wrapper($options['entity_type'], $options['root_id']);
      $current_bundle = $e->getBundle();
    }
    else
    {
      $current_bundle = array_keys($options['bundle_settings']);
      $current_bundle = current($current_bundle);
    }

    foreach($options['bundle_settings'] as $bundle => $data)
    {
      if($bundle == $current_bundle || $is_next == TRUE)
      {
        foreach($data['childs'] as $child)
        {
          $this->_options['parent_field'] = $child['field'];
          $this->_options['bundle'] = $child['bundle'];

          $result = $this->get_tree_data();
          if(isset($result) && !empty($result))
          {
            $options = $this->_options; // Сохраняем первоначальные опции

            foreach ($result as $id => $title)
            {
              //Новые опции для получения дочерних элементов 1-ого уровня
              $current_res[] = $id;

              $child_ids = global_tree_get_all_childs($id, $child['self_parent_field']);
              if (!(count($child_ids) == 1 && $child_ids[0] == ''))
              {
                //array_pop($child_ids);
                $child_ids = array_filter($child_ids);
              }
              $all_childs = array_merge($all_childs, $child_ids);

              //$this->_options['root_id'] = $id;
              //$this->_options['target_id'] = NULL;
              //$res[] = $this->get_tree_item_res();
            }

            $this->_options = $options; // Возвращаем первоначальные опции для дерева
          }
        }
      }

      if($is_next || !isset($this->_options['root_id'])) break;
      //else $is_next = TRUE;
    }

    $final_res = array_diff($current_res, $all_childs);

    $res = array();
    $options = $this->_options; // Сохраняем первоначальные опции
    foreach ($final_res as $id)
    {
      $this->_options['root_id'] = $id;
      $this->_options['target_id'] = NULL;
      $res[] = $this->get_tree_item_res();
    }
    $this->_options = $options; // Возвращаем первоначальные опции для дерева


    $this->_childs_count = count($res); // Количество детей для стилизации
    return $res;
  }



  /**
   * Получаем данные из БД
   * @return mixed
   * @throws \EntityFieldQueryException
   */
  protected function get_tree_data()
  {
    $query = db_select($this->_options['entity_type'], 'n');
    $query->fields('n', array('nid', 'title'));
    $query->condition('n.type', $this->_options['bundle']);
    $query->addTag('node_access');

    if ($this->_options['entity_type'] == 'node')
    {
      $query->condition('n.status', NODE_PUBLISHED); // Только опубликованные материалы

      // Сортировка
      // TODO: Сортировку по отдельному полю, нужно отдельно проработать
      if (isset($this->_options['order']['field']))
      {
        $order_type = 'ASC';
        if (isset($this->_options['order']['type'])) $order_type = $this->_options['order']['type'];

        $query->leftJoin('field_data_' . $this->_options['order']['field'], 'weight', 'weight.entity_id = n.nid');
        $query->orderBy("weight." . $this->_options['order']['field'] . "_value", $order_type);
      }
      else
      {
        $query->orderBy('n.title', 'ASC');
      }

      // Связь по родительскому полю
      if (isset($this->_options['parent_field']) && $this->_options['parent_field'])
      {
        if (isset($this->_options['root_id']) && $this->_options['root_id'])
        {
          $query->leftJoin('field_data_' . $this->_options['parent_field'], 'p', 'p.entity_id = n.nid');
          $query->condition('p.' . $this->_options['parent_field'] . '_' . $this->_options['entity_type_id'], $this->_options['root_id']);
          $query->condition("p.bundle", $this->_options['bundle']);
        }
        else
        {
          // Или корневые элементы без родителей
          $query->leftJoin('field_data_' . $this->_options['parent_field'], 'p', "n.nid = p.entity_id");
          $query->isNull('p.' . $this->_options['parent_field'] . '_' . $this->_options['entity_type_id']);
        }
      }
    }
    return $query->execute()->fetchAllKeyed();
  }



  /**
   * Функция получения непосредсвтвеных потомков объекта по его ID
   * @param $options атрибуты из GlobalTree, для нахождения потомков по связанным полям
   * @return array|bool массив детей, либо FALSE если непосредственные потомки отсутствуют
   */
  public static function get_childs($options)
  {
    if ($options['entity_type'] == 'node')
    {
      $is_next = FALSE;
      $count = 0;

      $e = entity_metadata_wrapper($options['entity_type'], $options['root_id']);
      $current_bundle = $e->getBundle();

      foreach($options['bundle_settings'] as $bundle => $data)
      {
        if($bundle == $current_bundle || $is_next == TRUE)
        {
          foreach($data['childs'] as $item)
          {
            $query = db_select($options['entity_type'], 'n');
            $query->fields('n', array('nid', 'title'));
            $query->addTag('node_access');
            $query->condition('n.type', $item['bundle']);
            $query->innerJoin('field_data_' . $item['field'], 'c', 'c.entity_id = n.nid');
            $query->condition("c.bundle", $item['bundle']);
            $query->condition('c.' . $item['field'] . '_' . $options['entity_type_id'], $options['root_id']);
            $childs = $query->countQuery()->execute()->fetchField();

            $count += $childs;
          }
        }

        if($is_next) break;
        else $is_next = TRUE;
      }

      return $count;
    }

    return FALSE;
  }


  public function get_tree_item_value($current_value, $nid, $title, $child_count) {
    return $current_value;
  }

  public function provide_json() {
    $data = self::get_tree_res();
    $json = self::form_json($data);
    drupal_add_js(array('globalTreeView' => array('json' => $json)), 'setting');
  }

  public function update_json($data, $nid, $position) {

  }

  public function form_json($items) {
    $arr = array();
    foreach ($items as $item) {
      $arr[$item['nid']] = (object)array(
        'title' => $item['title'],
        'level' => 1,
        'nid' => $item['nid'],
        'data' => array()
      );
    }
    return $arr;
  }
}
