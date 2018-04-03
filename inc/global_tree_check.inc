<?php

/**
 * Получить или установить статус: показывать галочки или нет
 * @param $options - boolean (true или false)
 * @return bool
 */
function global_tree_check_show($options = 2) {
  // Если пришло 1 (true) или 0 (false) значит устанавливаем соответствующий статус
  // Если ничего не пришло, значит нужно получить текущий статус
  if ($options == 2) {
    // Получить текущий статус
    if (isset($_SESSION['GLOBAL_TREE']['CHECK_SHOW'])) {
      return $_SESSION['GLOBAL_TREE']['CHECK_SHOW'];
    }
    else {
      return FALSE;
    }
  }
  else {
    $_SESSION['GLOBAL_TREE']['CHECK_SHOW'] = $options;
  }
}
