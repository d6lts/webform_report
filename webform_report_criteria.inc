<?php

/**
 * Generate a form for specifying report criteria.
 *
 * @param $form_state
 * @return an array of form elements
 */
function webform_report_criteria_form($form_state, $node) {
  
  global $user;

  module_load_include('inc', 'webform_report', 'webform_report');

  $mod_path = drupal_get_path('module','webform_report');
  $img_path = $mod_path . "/images/";

  if (isset($node->wnid)) {  
  
    $webform_components = _webform_report_get_components($node->wnid);
    
    $form['nid'] = array(
      '#type' => 'hidden',
      '#value' => $node->nid
    );
  
    $comp_options = array();
    $comp_options[0] = t('Select a field');
    foreach($webform_components as $cid => $comp) {
      $comp_options[$cid] = $comp['name'];
    }
  
    $component_date_format_options = array(
      '' => '', 
      'm/d/Y' => 'mm/dd/yyyy', 
      'd/m/Y' => 'dd/mm/yyyy', 
      'Y/m/d' => 'yyyy/mm/dd'
    );
    $component_time_format_options = array(
      '' => '', 
      'H:i' => 'hh:mm', 
      'h:i a' => 'hh:mm am/pm'
    );
  
    $columns = _webform_report_get_columns($node, $webform_components);
  
    // column fieldset  
    $form['columns'] = array(
      '#type' => 'fieldset', 
      '#title' => t('Report Columns'),
      '#description' => t('Columns are listed in same order as this list.'),
      '#tree' => TRUE
    );
  
    // form list of report columns
    $cc = count($columns);
    $i = 0;
    
    for ($i=0; $i<$cc; $i++) {
    
      $cid = $columns[$i]['cid'];
  
      $form['columns'][$i]['name'] = array(
        '#type' => 'markup',
        '#value' => $columns[$i]['name']
      );
      $form['columns'][$i]['type'] = array(
      '#type' => 'markup',
        '#value' => $columns[$i]['type']
      );
  
      if ($columns[$i]['type'] == 'date') {
        $form['columns'][$i]['format'] = array(
          '#type' => 'select',
          '#default_value' => $columns[$i]['format'],
          '#options' => $component_date_format_options
        );
      }
      if ($columns[$i]['type'] == 'time') {
        $form['columns'][$i]['format'] = array(
          '#type' => 'select',
          '#default_value' => $columns[$i]['format'],
          '#options' => $component_time_format_options
        );
      }
  
      webform_report_criteria_form_list_buttons($form, $img_path, 'columns', $cid, $i, $cc);
    }

    $form['columns']['addcomp'] = array(
      '#type' => 'select',
      '#options' => $comp_options,
      '#required' => FALSE
    );
  
    $form['columns']['add'] = array(
      '#type' => 'submit',
      '#value' => t('Add Column'),
      '#return_value' => 'columns_add',
      '#submit' => array('webform_report_criteria_form_list_submit')
    );
  
    $filters = _webform_report_get_filters($node, $webform_components);
    
    $filter_options = array(
      0 => t('none'), 
      1 => t('begins with'), 
      2 => t('does not begin with'), 
      3 => t('contains'), 
      4 => t('does not contain'), 
      5 => t('equals'), 
      6 => t('does not equal'), 
      7 => t('is empty'), 
      8 => t('is not empty'),
      9 => t('greater than'), 
      10 => t('less than')
    );
    
    // filters fieldset  
    $form['filters'] = array(
      '#type' => 'fieldset', 
      '#title' => t('Filters'),
      '#description' => t('All filters are ANDed together.'),
      '#tree' => TRUE
    );
  
    // form list of report filters
    $cc = count($filters);
    $i = 0;
    
    for ($i=0; $i<$cc; $i++) {
  
      $cid = $filters[$i]['cid'];
  
      $form['filters'][$i]['name'] = array(
        '#type' => 'markup',
        '#value' => $filters[$i]['name']
      );
      $form['filters'][$i]['type'] = array(
        '#type' => 'markup',
        '#value' => $filters[$i]['type']
      );
  
      $form['filters'][$i]['ftype'] = array(
        '#type' => 'select',
        '#options' => $filter_options,
        '#default_value' => $filters[$i]['ftype']
      );
  
      $form['filters'][$i]['value'] = array(
        '#type' => 'textfield',
        '#default_value' => $filters[$i]['value']
      );
      
      webform_report_criteria_form_list_buttons($form, $img_path, 'filters', $cid, $i, $cc);
    }
    
    $form['filters']['addcomp'] = array(
      '#type' => 'select',
      '#options' => $comp_options,
      '#required' => FALSE
    );
  
    $form['filters']['add'] = array(
      '#type' => 'submit',
      '#value' => t('Add Filter'),
      '#return_value' => 'filters_add',
      '#submit' => array('webform_report_criteria_form_list_submit')
    );
  
    $sorton = _webform_report_get_sorton($node, $webform_components);
  
    $sort_options = array(
      SORT_ASC => t('Ascending'), 
      SORT_DESC => t('Descending')
    );
    
    // sort fieldset  
    $form['sorton'] = array(
      '#type' => 'fieldset', 
      '#title' => t('Sort By'),
      '#description' => t('Sorting is applied in same order as this list.'),
      '#tree' => TRUE
    );
  
    // form list of report sort options
    $cc = count($sorton);
    $i = 0;
    
    for ($i=0; $i<$cc; $i++) {
  
      $cid = $sorton[$i]['cid'];
  
      $form['sorton'][$i]['name'] = array(
        '#type' => 'markup',
        '#value' => $sorton[$i]['name']
      );
      $form['sorton'][$i]['type'] = array(
        '#type' => 'markup',
        '#value' => $sorton[$i]['type']
      );
  
      $form['sorton'][$i]['order'] = array(
        '#type' => 'select',
        '#options' => $sort_options,
        '#default_value' => $sorton[$i]['order']
      );
  
      webform_report_criteria_form_list_buttons($form, $img_path, 'sorton', $cid, $i, $cc);
    }
    
    $form['sorton']['addcomp'] = array(
      '#type' => 'select',
      '#options' => $comp_options,
      '#required' => FALSE
    );
  
    $form['sorton']['add'] = array(
      '#type' => 'submit',
      '#value' => t('Add Sort'),
      '#return_value' => 'sorton_add',
      '#submit' => array('webform_report_criteria_form_list_submit')
    );
  
    // options fieldset  
    $form['options'] = array(
      '#type' => 'fieldset', 
      '#title' => t('Options'),
      '#description' => t('Report options.')
    );
  
    $results_per_page_options = array(
      0 => 'All', 
      20 => '20', 
      40 => '40', 
      60 => '60', 
      80 => '80', 
      100 => '100'
    );
    $default = 20;
    if (array_key_exists('results_per_page', $node->options)) {
      $default = $node->options['results_per_page'];
    }
    $form['options']['results_per_page'] = array(
      '#type' => 'select',
      '#title' => t('Results per page'),
      '#options' => $results_per_page_options,
      '#default_value' => $default,
    );

    $default = FALSE;  
    if (array_key_exists('hide_csv', $node->options)) {
      $default = $node->options['hide_csv'];
    }
    $form['options']['hide_csv'] = array(
      '#type' => 'checkbox',
      '#title' => t('Hide CSV Download option'),
      '#default_value' => $default,
    );
    
    $form['update'] = array(
      '#type' => 'submit',
      '#value' => t('Update'),
    );
  
    return $form;
  }
  else {
    drupal_set_message(t('Error - Webform is not set for this report - cannot proceed.  Please delete and re-create this report.'), 'error');
    return NULL;
  }
}

function webform_report_criteria_form_list_buttons(&$form, $img_path, $list, $cid, $index, $count) {

  $form[$list][$index]['delete'] = array(
    '#type' => 'image_button',
    '#src' => $img_path . 'trash.png',
    '#return_value' => $list . '_delete_' . $index . '_' . $cid,
    '#attributes' => array('alt' => t('Delete this item')),
    '#submit' => array('webform_report_criteria_form_list_submit')
  );
  
  if ($index > 0) {
    $form[$list][$index]['top'] = array(
      '#type' => 'image_button',
      '#src' => $img_path . 'toparrow.png',
      '#return_value' => $list . '_top_' . $index . '_' . $cid,
      '#attributes' => array('alt' => t('Move item to the top')),
      '#submit' => array('webform_report_criteria_form_list_submit')
    );
    $form[$list][$index]['up'] = array(
      '#type' => 'image_button',
      '#src' => $img_path . 'uparrow.png',
      '#return_value' => $list . '_up_' . $index . '_' . $cid,
      '#attributes' => array('alt' => t('Move item up')),
      '#submit' => array('webform_report_criteria_form_list_submit')
    );
  }
  else {
    $form[$list][$index]['top'] = array(
      '#type' => 'markup'
    );
    $form[$list][$index]['up'] = array(
      '#type' => 'markup'
    );
  }
  if ($index < $count-1) {
    $form[$list][$index]['down'] = array(
      '#type' => 'image_button',
      '#src' => $img_path . 'downarrow.png',
      '#return_value' => $list . '_down_' . $index . '_' . $cid,
      '#attributes' => array('alt' => t('Move item down')),
      '#submit' => array('webform_report_criteria_form_list_submit')
    );
    $form[$list][$index]['end'] = array(
      '#type' => 'image_button',
      '#src' => $img_path . 'endarrow.png',
      '#return_value' => $list . '_end_' . $index . '_' . $cid,
      '#attributes' => array('alt' => t('Move item to the end')),
      '#submit' => array('webform_report_criteria_form_list_submit')
    );
  }
  else {
    $form[$list][$index]['down'] = array(
      '#type' => 'markup'
    );
    $form[$list][$index]['end'] = array(
      '#type' => 'markup'
    );
  }
}

/**
 * Theme the report criteria form
 */
function theme_webform_report_criteria_form($form) {

  if (array_key_exists('columns', $form)) {
  
    // theme column list
    $header = array(
      t('Name'), 
      t('Type'), 
      t('Format'), 
      array('data' => t('Operations'), 'colspan' => '5')
    );
    
    $rows = array();
    
    foreach (element_children($form['columns']) as $index) {
  
      $rows[] = array(
        drupal_render($form['columns'][$index]['name']),
        drupal_render($form['columns'][$index]['type']),
        drupal_render($form['columns'][$index]['format']),
        drupal_render($form['columns'][$index]['delete']),    
        drupal_render($form['columns'][$index]['top']), 
        drupal_render($form['columns'][$index]['up']),
        drupal_render($form['columns'][$index]['down']),
        drupal_render($form['columns'][$index]['end'])    
      );
    }
  
    $form['columns']['table'] = array(
      '#value' => theme('table', $header, $rows)
    );
  
    $header = array(
      array('data' => t('Add Column'), 'colspan' => '2')
    );
  
    $rows = array();
    $rows[] = array(
      drupal_render($form['columns']['addcomp']), 
      drupal_render($form['columns']['add'])
    );
    
    $form['columns']['addtable'] = array(
      '#value' => theme('table', $header, $rows)
    );
  }
  
  if (array_key_exists('filters', $form)) {
  
    // theme filter list
    $header = array(
      t('Name'), 
      t('Type'), 
      t('Filter Type'), 
      t('Filter Value'), 
      array('data' => t('Operations'), 'colspan' => '5')
    );
    
    $rows = array();
    
    foreach (element_children($form['filters']) as $index) {
  
      $rows[] = array(
        drupal_render($form['filters'][$index]['name']),
        drupal_render($form['filters'][$index]['type']),
        drupal_render($form['filters'][$index]['ftype']),
        drupal_render($form['filters'][$index]['value']),
        drupal_render($form['filters'][$index]['delete']),    
        drupal_render($form['filters'][$index]['top']), 
        drupal_render($form['filters'][$index]['up']),
        drupal_render($form['filters'][$index]['down']),
        drupal_render($form['filters'][$index]['end'])    
      );
    }
  
    $form['filters']['table'] = array(
      '#value' => theme('table', $header, $rows)
    );
  
    $header = array(
      array('data' => t('Add Filter'), 'colspan' => '2')
    );
  
    $rows = array();
    $rows[] = array(
      drupal_render($form['filters']['addcomp']),
      drupal_render($form['filters']['add'])
    );
    
    $form['filters']['addtable'] = array(
      '#value' => theme('table', $header, $rows)
    );
  }
   
  if (array_key_exists('sorton', $form)) {
  
    // theme sort list
    $header = array(
      t('Name'), 
      t('Type'), 
      t('Order'), 
      array('data' => t('Operations'), 'colspan' => '5')
    );
    
    $rows = array();
    
    foreach (element_children($form['sorton']) as $index) {
  
      $rows[] = array(
        drupal_render($form['sorton'][$index]['name']),
        drupal_render($form['sorton'][$index]['type']),
        drupal_render($form['sorton'][$index]['order']),
        drupal_render($form['sorton'][$index]['delete']),    
        drupal_render($form['sorton'][$index]['top']), 
        drupal_render($form['sorton'][$index]['up']),
        drupal_render($form['sorton'][$index]['down']),
        drupal_render($form['sorton'][$index]['end'])    
      );
    }
  
    $form['sorton']['table'] = array(
      '#value' => theme('table', $header, $rows)
    );
  
    $header = array(
      array('data' => t('Add Sort'), 'colspan' => '2')
    );
  
    $rows = array();
    $rows[] = array(
      drupal_render($form['sorton']['addcomp']),
      drupal_render($form['sorton']['add'])
    );
  
    $form['sorton']['addtable'] = array(
      '#value' => theme('table', $header, $rows)
    );
  }
  
  return drupal_render($form);
}

/**
 * Implementation of hook_validate
 */
function webform_report_criteria_form_validate($form_id, $form_state) {
  
}

function webform_report_criteria_form_list_submit($form_id, $form_state) {

  // load the node
  $nid = $form_state['values']['nid'];
  $node = node_load($nid);

  // get the return value of the button that was pressed
  $return = $form_state['clicked_button']['#return_value'];
  // extract the list, operation, index, and cid
  $values = explode('_', $return);
  $list = $values[0];
  $op = $values[1];
  if (count($values) >= 3) {
    $index = (int) $values[2];
  }
  if (count($values) >= 4) {
    $cid = (int) $values[3];
  }

  $upd = FALSE;
  
  // perform the operation    
  if ($op == 'add') {
    $cid = (int) $form_state['values'][$list]['addcomp'];
    if ($cid != 0) {
      if ($list == 'columns') {
        $node->columns[] = array(
          'cid' => $cid,
          'format' => ''
        );
        $upd = TRUE;
      }
      else if ($list == 'sorton') {
        $node->sorton[] = array(
          'cid' => $cid,
          'order' => SORT_ASC
        );
        $upd = TRUE;
      }
      else if ($list == 'filters') {
        $node->filters[] = array(
          'field' => $cid,
          'type' => 0,
          'value' => ''
        );
        $upd = TRUE;
      }
    }
  }
  
  else if ($op == 'delete' && $index >= 0) {
    if ($list == 'columns') {
      $node->columns = array_values(array_diff_key($node->columns, array($index => $index)));
      $upd = TRUE;
    }
    else if ($list == 'sorton') {
      $node->sorton = array_values(array_diff_key($node->sorton, array($index => $index)));
      $upd = TRUE;
    }
    else if ($list == 'filters') {
      $node->filters = array_values(array_diff_key($node->filters, array($index => $index)));
      $upd = TRUE;
    }
  }
  
  else {
    
    if ($list == 'columns') {
      $clist = $node->columns;
    }
    else if ($list == 'sorton') {
      $clist = $node->sorton;
    }
    else if ($list == 'filters') {
      $clist = $node->filters;
    }
    
    if ($op == 'top' && $cid != 0) {
      if ($index > 0) {
        for ($i = $index; $i > 0; $i--) {
          $tmp = $clist[$i];
          $clist[$i] = $clist[$i-1];
          $clist[$i-1] = $tmp;
        }
        $upd = TRUE;
      }
    }
  
    else if ($op == 'up' && $cid != 0) {
      if ($index > 0) {
          $tmp = $clist[$index];
          $clist[$index] = $clist[$index-1];
          $clist[$index-1] = $tmp;
      }
      $upd = TRUE;
    }
  
    else if ($op == 'down' && $cid != 0) {
      if ($index >= 0 && $index < count($clist)) {
          $tmp = $clist[$index];
          $clist[$index] = $clist[$index+1];
          $clist[$index+1] = $tmp;
      }
      $upd = TRUE;
    }
  
    else if ($op == 'end' && $cid != 0) {
      if ($index >= 0 && $index < count($clist)) {
        for ($i = $index; $i < count($clist) - 1; $i++) {
          $tmp = $clist[$i];
          $clist[$i] = $clist[$i+1];
          $clist[$i+1] = $tmp;
        }
        $upd = TRUE;
      }
    }
    
    if ($upd) {
      if ($list == 'columns') {
        $node->columns = $clist;
      }
      else if ($list == 'sorton') {
        $node->sorton = $clist;
      }
      else if ($list == 'filters') {
        $node->filters = $clist;
      }
    }
  }

  // update the node  
  if ($upd) {
    node_save($node);
  }
}

/**
 * Implementation of hook_submit
 */
function webform_report_criteria_form_submit($form_id, $form_state) {

  // load the node
  $nid = $form_state['values']['nid'];
  $node = node_load($nid);

  // update lists
  foreach (element_children($form_state['values']['columns']) as $index) {
    if (is_numeric($index)) {
      if (isset($form_state['values']['columns'][$index]['format'])) {
        if (array_key_exists($index, $node->columns)) {
          $node->columns[$index]['format'] = $form_state['values']['columns'][$index]['format'];
        }
      }
    }
  }

  foreach (element_children($form_state['values']['filters']) as $index) {
    if (is_numeric($index)) {
      if (array_key_exists($index, $node->filters)) {
        $node->filters[$index]['type'] = $form_state['values']['filters'][$index]['ftype'];
        $node->filters[$index]['value'] = $form_state['values']['filters'][$index]['value'];
      }
    }
  }

  foreach (element_children($form_state['values']['sorton']) as $index) {
    if (is_numeric($index)) {
      if (array_key_exists($index, $node->sorton)) {
        $node->sorton[$index]['order'] = $form_state['values']['sorton'][$index]['order'];
      }
    }
  }

  // update options
  $node->options['results_per_page'] = $form_state['values']['results_per_page'];
  $node->options['hide_csv'] = $form_state['values']['hide_csv'];

  // update the node  
  node_save($node);

  drupal_set_message(t('Webform Report has been updated.'));
}