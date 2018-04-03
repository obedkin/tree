(function ($) {

    Drupal.behaviors.custom_tree = {

        attach: function (context, settings) {


            // Если информацию с сервера уже подгружали
            $('ul.tree li.load div a.icon-open').once(function () {
                $(this).click(function (event) {

                    if (!$(this).hasClass('use-ajax')) {
                        if (!$(this).closest('.load').hasClass('opened')) {
                            $(this).closest('.closed').removeClass('closed').addClass('opened');
                            $(this).closest('.tree_item').next().show();
                            if ($(this).closest('.load').hasClass('empty')) {
                                $(this).html('<div class="i-open i-empty"></div><span class="fa fa-folder-open-o"></span>');
                            } else {
                                $(this).html('<div class="i-open i-full"></div><span class="fa fa-folder-open"></span>');
                            }
                        } else {
                            $(this).closest('.opened').removeClass('opened').addClass('closed');
                            $(this).closest('.tree_item').next().hide();
                            if ($(this).closest('.load').hasClass('empty')) {
                                $(this).html('<div class="i-close i-empty"></div><span class="fa fa-folder-o"></span>');
                            } else {
                                $(this).html('<div class="i-close i-full"></div><span class="fa fa-folder"></span>');
                            }
                        }
                        event.preventDefault();
                        return false;
                    }
                })
            })


            //Крутилка для элементов деревая, которые будут грузиться через AJAX
            $('.use-ajax .i-close.i-full').once(function () {
                $(this).click(function (event) {
                    $(this).html('<img src="' + Drupal.settings.basePath + Drupal.settings.module_path + '/img/spin.gif">');
                });
            });


            //Кнопка для работы с объектами
            $('.object-icon').once(function () {
                $(this).click(function () {
                    $(this).toggleClass('fa-angle-double-down');
                    $(this).toggleClass('fa-angle-double-up');
                })
            });


            //Закрытие коллапса на кнопку "Нет" или "Отмена"
            $('.btn-close').once(function () {
                $(this).click(function () {
                    $('.item_forms .collapse').hide();
                    $('.tree_btn').removeClass("active");
                })
            });


            //Форма добавления новых элементов в дерево
            $('.form-add-title').once(function () {
                $(this).keyup(function () {
                    var url = Drupal.ajax['form-add-href-' + $(this).data('nid')].options.url;
                    Drupal.ajax['form-add-href-' + $(this).data('nid')].options.url = url + '&title=' + $(this).val();
                })
            });


            // Ставим фокус в форму добавления
            $('.btn_add').once(function () {
                $(this).click(function () {
                    //Сразу фокус не ставиться, видимо из-за того, что input'а еще нет на экране
                    var input_focus = $(this).closest('.tree_item, .tree-top .form-add').find('input').first();
                    setTimeout(function () {
                        $(input_focus).focus()
                    }, 100);
                })
            });

            // Фокус в верхнюю форму добавления
            $('.top-buttons .btn_add').once(function () {
                $(this).click(function () {
                    var input_focus = $('.form-add').find('input').first();
                    setTimeout(function () {
                        $(input_focus).focus()
                    }, 100);
                })
            });

            // Фокус при редактировании
            $('.btn_edit').once(function () {
                $(this).click(function () {
                    var form_id = $(this).data('form-id');
                    var input_focus = $('#' + form_id).find('input').first();
                    setTimeout(function () {
                        $(input_focus).focus()
                    }, 100);
                })
            })


            // Сортировка (https://github.com/mjsarfatti/nestedSortable)
            $('.btn_sort').once(function () {
                $(this).click(function () {
                    $('.sortable').toggleClass('in');
                    if ($('.sortable').hasClass('in')) {
                        $('.sortable').nestedSortable({
                            listType: 'ul',
                            handle: 'div',
                            items: 'li:not(.no-sortable)',
                            isTree: true,
                            //toleranceElement: '> div',
                            stop: function () {
                                tree_save();
                            }
                        });
                        console.log('Сортировка: вкл');
                    } else {
                        $(".sortable").nestedSortable('destroy');
                        console.log('Сортировка: выкл');
                    }
                    return false;
                })
            });

            // Cохранение последовательности в дереве при drag&drop сортировке
            function tree_save() {
                //var serialized = $('.sortable').nestedSortable('serialize');
                //console.log(serialized);
                //var order_field = $('.sortable').data('order-field');
                //var parent_field = $('.sortable').data('parent-field');
                //var root_id = $('.sortable').attr('id');
                //var url = '/global_tree/sortable/' + order_field + '&' + parent_field + '&' + root_id + '&' + serialized;
                //console.log('Сохранение дерева: ' + url);

                
                // Теперь используем JQuery
                $.ajax({
                    url: Drupal.settings.basePath + 'global_tree/sortable/',
                    type: "POST",
                    data:{
                        'order_field': $('.sortable').data('order-field'),
                        'parent_field' : $('.sortable').data('parent-field'),
                        'root_id' : $('.sortable').attr('id'),
                        'serialize': $('.sortable').nestedSortable('serialize')
                    },
                    success: function (result) {
                        console.log(result)
                    }
                });
            }

            // Поиск
            if ($('#autocomplete').length) {
                self = $(this);
                $("#autocomplete").autocomplete({
                    search: function (event, ui) {
                        //Включаем крутилку
                        $(this).parent().find('.glyphicon-refresh').addClass('glyphicon-spin');
                        //Выключаем крутилку
                        setTimeout(function () {
                            $(this).parent().find('.glyphicon-refresh').removeClass('glyphicon-spin');
                        }, 10000)
                    },
                    source: function (request, response) {
                        // организуем кроссдоменный запрос
                        $.ajax({
                            url: Drupal.settings.basePath + "global_tree/search/title=" + request.term + '&' + $('.top-search-list').data('options'),
                            dataType: "json",
                            // параметры запроса, передаваемые на сервер (последний - подстрока для поиска):
                            data: {
                                featureClass: "P",
                                style: "full",
                                maxRows: 12,
                                name_startsWith: request.term
                            },
                            // обработка успешного выполнения запроса
                            success: function (data) {
                                $('.glyphicon-refresh').removeClass('glyphicon-spin');//Выключаем крутилку
                                console.info(data);
                                create_top_search_list(data);
                                $('.top-search-list').show(100);
                            },
                            error: function (data) {
                                console.error(data);
                            }
                        });
                    },
                    minLength: 2
                });
            }


            // Формируем выпадающий список
            function create_top_search_list(data) {
                var list = '<ul>';
                for (var key in data) {
                    list += '<li data-id="' + key + '">' + data[key] + '</li>';
                }
                list += '</ul>';
                $('.top-search-list').html(list);
                $('.top-search-list ul li').once(function () {
                    $(this).click(function () {
                        window.location = Drupal.settings.basePath + $('.top-search-list').data('url') + "/" + $(this).data('id');
                    })
                });
            }


            // Прячем выпадающий список при клике по любому месту страницы
            $(function () {
                $(document).click(function (event) {
                    if ($(event.target).closest(".top-search-list").length) return;
                    $(".top-search-list").hide(100);
                    event.stopPropagation();
                });
            });

            // Клики по кнопка управления элементами дерева
            $('.tree_btn').once(function () {
                $(this).click(function () {
                    $('.item_forms .collapse').hide();
                    var is_active = 0;
                    if ($(this).hasClass('active')) {
                        is_active = 1;
                    }

                    // Отдельно обрабатываем кнопки "Сортировка" и "Выборка"
                    if ($(this).hasClass('btn_sort') || $(this).hasClass('btn_check')) {
                        $(this).toggleClass("active");
                    } else {
                        $('.tree_btn').removeClass("active");
                        if (is_active == 0) {
                            $(this).addClass('active')
                        } else {
                            $('.collapse').removeClass('in');
                            return false;
                        }
                    }

                })
            });

            // Эмулятор bootstrap'овского collapse
            $('.tree-top .collapse, .item_actions .collapse').once(function () {
                $(this).click(function () {
                    $('#' + $(this).data('collapse')).toggle();
                    if ($(this).hasClass('tree_btn') && !$(this).hasClass('active')) {
                        $('#' + $(this).data('collapse')).hide();
                    }
                    return false;
                })
            });

            // Submit по иконке
            $('.span-submit').once(function () {
                $(this).click(function () {
                    $(this).find('.fa').addClass('fa-spin');
                    $(this).find('input,button').click();
                    return false;
                })
            });

            // Или submit обыкновенный
            $(".form-add input").keypress(function (event) {
                if (event.which == 13) {
                    $('.form-add').find('.fa').addClass('fa-spin');
                }
            });


            // CHECK ---------------------------------------------------------------------------------------------------
            // Вкл/выкл кнопки check в шапке дерева
            $('.btn_check').once(function () {
                $(this).click(function () {
                    $('.label-check').toggleClass('hide');
                    if ($(this).hasClass('active')) {
                        $.ajax({
                            url: Drupal.settings.basePath + 'global_tree/check/1',
                            success: function (result) {
                                console.log('Включена выборка элементов дерева');
                            }
                        });

                    } else {
                        $.ajax({
                            url: Drupal.settings.basePath + 'global_tree/check/0',
                            success: function (result) {
                                console.log('Выключена выборка элементов дерева');
                            }
                        });
                    }
                    return false;
                });
            });

            // 1 check в строке дерева
            $('.item-check').once(function () {
                $(this).click(function () {
                    $(this).toggleClass("fa-square-o");
                    $(this).toggleClass("fa-check-square-o");
                    var id = $(this).data('id');
                    global_tree_list(id);
                });
            });

            // Добавляем или удаляем ID из указанного селектора
            function global_tree_list(id) {
                var out_selector = $('.btn_check').data('out-selector');
                var list = $(out_selector).val().trim();
                // Пустая строка
                if (list == '') {
                    $(out_selector).val(id);
                } else {
                    list = list.split(',');
                    ar_list = [];
                    list.forEach(function (item) {
                        ar_list[item] = item
                    });
                    if (id in ar_list) {
                        delete ar_list[id];
                    } else {
                        ar_list[id] = id;
                    }
                    list = '';
                    ar_list.forEach(function (item) {
                        list += item + ',';
                    });
                    $(out_selector).val(list.substring(0, list.length - 1));
                }
            }
        }
    };
})(jQuery);
