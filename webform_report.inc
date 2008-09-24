<?php

// $Id$

/**
 * Searches for nodes of type webform and returns their nids and titles
 * in an associative array
 */
function _webform_report_get_webforms() {
  $result = db_query("SELECT nid FROM {node} WHERE type = 'webform'");

  while ($row = db_fetch_object($result)) {
    $webform_report = node_load($row->nid); // load the current version of this node
    $webforms[$webform_report->nid] = t($webform_report->title);
  }
  return $webforms;
} // end function _webform_report_get_webforms()

/**
 * Get webform components
 *
 * @param nid is the node id of a webform
 * @param default (optional) is the first item in the returned array,
 *   for use with form components
 * @return an associative array of webform components
 */
function _webform_report_get_components($nid) {
  $components = array();
  $result = db_query("SELECT c.cid, c.name FROM {webform_component} c WHERE c.type <> 'fieldset' AND c.nid = '$nid' ORDER BY c.weight");
  
  while ($row = db_fetch_object($result)) {
    $component_name = substr($row->name, 0, 65);
    if (strlen($row->name) > 65) {
      $component_name .= "...";
    }
    $components[$row->cid] = t($component_name);
  }
  
  return $components;
}


/**
 * Get data for the specified webform
 *
 * @param node is an object of type node
 * @return a database query result set
 */
function _webform_report_get_data($node) {
  //$result = db_query("SELECT w.nid, c.name FROM {webform} w LEFT JOIN {webform_component} c ON w.nid = c.nid");

  
  $result = db_query("SELECT w.nid, c.name, c.cid, c.type, d.nid, d.sid, d.data, s.uid, u.name as user, s.submitted, s.remote_addr
                        FROM {webform} w LEFT JOIN {webform_submitted_data} d ON w.nid = d.nid LEFT JOIN {webform_component} c ON d.cid = c.cid LEFT JOIN {webform_submissions} s
                        ON d.sid = s.sid LEFT JOIN {users} u ON s.uid = u.uid WHERE d.nid = c.nid AND c.nid = s.nid AND s.nid = '" . $node->wnid . "' ORDER BY d.sid, c.cid, c.name, d.data");
  return $result;  
}

/**
 * Get node body content for the specified webform report
 *
 * @param data is a database query result set
 * @param node is the current node object
 * @return a string of text or a themed table
 */
function _webform_report_get_body_content($data, $node) {
  if ($data) {
    $fields = array(); // webform field names
    $values = array(); // webform field values
    $last_value = 0;

    // system fields
    $fields[-1] = array('data' => t('User'), 'field' => -1, 'sort' => $_GET['sort']);
    $fields[-2] = array('data' => t('Date'), 'field' => -2, 'sort' => $_GET['sort']);
    $fields[-3] = array('data' => t('Time'), 'field' => -3, 'sort' => $_GET['sort']);
    $fields[-4] = array('data' => t('IP Address'), 'field' => -4, 'sort' => $_GET['sort']);
    $fields[-5] = array('data' => t('Edit'), 'field' => -5, 'sort' => $_GET['sort']);
    // begin fields
    while ($row = db_fetch_object($data)) {
      if(!isset($fields[$row->cid])) {
        $fields[$row->cid] = array('data' => $row->name, 'field' => $row->cid, 'sort' => $_GET['sort']);
      } // end fields
      // begin data
      if ($row->data != '0' && !empty($last_value)) { // checkbox, not selected
        if($row->cid == $last_cid) { // will comma-separate multiple selections
          $row->data .= ", " . $last_value;
        }
        if($row->user == '') {
          $row->user = t('anonymous');
        }
        $values[$row->sid][-1] = array('data' => $row->user, 'field' => -1);
        $values[$row->sid][-2] = array('data' => date('Y-m-d', $row->submitted), 'field' => -2);
        $values[$row->sid][-3] = array('data' => date('H:s', $row->submitted), 'field' => -3);
        $values[$row->sid][-4] = array('data' => $row->remote_addr, 'field' => -4);
        $values[$row->sid][-5] = array('data' => '<a href="' . url('node/'. $row->nid . '/submission/' . $row->sid . '/edit') . '">' . t('edit') .   '</a>', 'field' => -5);
        $values[$row->sid][$row->cid] = array('data' => $row->data, 'field' => $row->cid); // preserve component id
      } else {
        $values[$row->sid][$row->cid] = array('data' => '&nbsp;', 'field' => $row->cid); // prevents the table cell from being omitted
      }

      // override the report's sort column with the table sort column, if applicable
      if (isset($_GET['order'])) {
        if ($_GET['order'] == $row->name && $node->kcid != $row->cid) {
          $node->sort_col = $row->cid;
        }
      }
      else {
        // no table sort link has been clicked yet
        switch($node->kcid) {
          case -1:
            $_GET['order'] = t('User');
            break;
          case -2:
            $_GET['order'] = t('Date');
            break;
          case -3:
            $_GET['order'] = t('Time');
            break;
          case -4:
            $_GET['order'] = t('IP Address');
            break;
          case $row->cid:
            $_GET['order'] = $row->name; // make the sort arrow marker appear in the table column header
            break;
        }
      }
      $last_cid = $row->cid;
      $last_value = $row->data;
    }

    reset($fields); // top

    // get an array of columns for sorting
    $column = array();
    foreach ($fields as $key1 => $field) {
      foreach ($values as $key2 => $value) {
        // make sort order case-insensitive and remove spaces (and accents if accents module installed)
        if (module_exists('accents')) {
          $column[$key1][$key2] = trim(strtolower(accents_search_preprocess($value[$key1]['data'])));
        }
        else {
          $column[$key1][$key2] = trim(strtolower($value[$key1]['data']));
        }
      }
    }

    // override the report's sort order with the table sort order, if applicable
    if (isset($_GET['sort'])) { 
      switch ($_GET['sort']) {
        case 'asc':
          $node->sort = SORT_ASC;
          break;
        case 'desc':
          $node->sort = SORT_DESC;
          break;
      }
    }
    else { // override the table's sort order with the report's
      if ($node->sort == SORT_ASC) {
        $_GET['sort'] = 'asc'; // default table sort will be ascending
      }
      else {
        $_GET['sort'] = 'desc'; // default table sort will be descending
      }
    }

    // handle the sorting of optional, non-component fields
    switch($_GET['order']) {
      case t('User'):
        $node->sort_col = -1;
        $sort = SORT_STRING;
        break;
      case t('Date'):
        $node->sort_col = -2;
        $sort = SORT_REGULAR;
        break;
      case t('Time'):
        $node->sort_col = -3;
        $sort = SORT_REGULAR;
        break;
      case t('IP Address'):
        $node->sort_col = -4;
        $sort = SORT_REGULAR;
        break;
      default:
        $sort = SORT_REGULAR;
    }

    // sort the columns and their contents according to specified criteria
    array_multisort($column[$node->sort_col], (int)$node->sort, $values);

    _webform_report_prepare_report_data(&$fields, &$values, $node); // keep only fields requested in report criteria

    // filter the table values
    if($node->filter_type != 0) {
      $values = _webform_report_filter_values($values, $node);
    }

    $values = _webform_report_add_data_links($fields, $values);

    // display number of rows after description
    $output .= " (" . count($values) . " " . t('results') . ")</p>";

    $output .= _webform_report_pager($fields, $values, $node);
  }
  else { // no submitted data
    $output = t('Note: There are no submissions for the selected webform.</b> Either the form
       has not yet been completed by anyone, or the results have been cleared. This will not
       prevent you from creating this report, but this message will be displayed on the report
       page until someone submits the selected webform.');
  }
  return $output;
}

function _webform_report_prepare_report_data(&$fields, &$values, $node) {
  $new_fields = array();
  $new_values = array();
  foreach($fields as $key1 => $field) {
    if(in_array($key1, $node->components)) {
      $new_fields[$key1] = $fields[$key1];
    }
    foreach($values as $key2 => $value) {
      if(in_array($value[$key1]['field'], $node->components)) {
        $new_values[$key2][$key1] = $value[$key1];
      }
    }
  }
  $fields = $new_fields;
  $values = $new_values;
}

/**
 * Add links to relevant data fields
 * @param fields is an array of table fields
 * @param values is an array of table values
 * @return an array of table values
 */
function _webform_report_add_data_links($fields, $values) {
  foreach($fields as $key1 => $field) {
    foreach($values as $key2 => $value) {
      $data = &$values[$key2][$key1]['data']; // shorten access to the data element

      if($key1 == -1) {
        $data = '<a href="' . url('user/' . _webform_report_get_uid_for_user($data)) . '">' . $data . '</a>';
      } else {
        if(valid_email_address($data)) {
          $data = '<a href="mailto:' . $data . '">' . $data . '</a>';
        } else {
          if(valid_url($data, TRUE)) {
            $data = '<a href="' . $data . '">' . $data . '</a>';
          }
        }
      }
    }
  }
  return $values;
} // function _webform_report_add_data_links

/**
 * Filter an array of table values
 * @return a filtered array
 */
function _webform_report_filter_values($values, $node) {
  $filtered_values = array();
  foreach($values as $key => $value) {
    switch($node->filter_type) {
      case 1: // begins with
        if(substr(strtolower(trim($value[$node->kcid]['data'])), 0, strlen($node->filter_value)) == strtolower($node->filter_value)) {
          $filtered_values[] = $value;
        }
        break;
      case 2: // does not begin with
        if(substr(strtolower(trim($value[$node->kcid]['data'])), 0, strlen($node->filter_value)) != strtolower($node->filter_value)) {
          $filtered_values[] = $value;
        }
        break;
      case 3: // contains
        if(strpos(strtolower(trim($value[$node->kcid]['data'])), strtolower($node->filter_value)) !== FALSE) {
          $filtered_values[] = $value;
        }
        break;
      case 4: // does not contain
        if(strpos(strtolower(trim($value[$node->kcid]['data'])), strtolower($node->filter_value)) === FALSE) {
          $filtered_values[] = $value;
        }
        break;
      case 5: // equals
        if(strtolower(trim($value[$node->kcid]['data'])) == strtolower(trim($node->filter_value))) {
          $filtered_values[] = $value;
        }
        break;
      case 6: // does not equal
        if(strtolower(trim($value[$node->kcid]['data'])) != strtolower(trim($node->filter_value))) {
          $filtered_values[] = $value;
        }
        break;
    }
  }

  return $filtered_values;
}

/**
 * Returns the uid corresponding with a valid username
 * @param a string consisting of a valid username
 * @return a uid for the specified user
 */
function _webform_report_get_uid_for_user($name) {
  $result = db_query("SELECT uid FROM users WHERE name = '" . $name . "'");
  $user = db_fetch_object($result);
  return $user->uid;
} // function webform_report_get_uid_for_user

/**
 * Display a list of webform reports in the admin area
 * @return a themed table
 */
function _webform_report_page() {
  $output = NULL;
  if (user_access('access webform reports')) {
    $header = array(
                    t('View'), t('Edit'), t('Delete')
                    );

    $result = db_query("SELECT nid, title FROM {node} WHERE type='webform_report'");
   
    while ($node = db_fetch_object($result)) {
      $rows[] = array(l($node->title,'node/' . $node->nid),
                      l(t('edit'), 'node/'.$node->nid.'/edit'),
                      l(t('delete'), 'node/'.$node->nid.'/delete'));

    }
  
    $output = theme_table($header, $rows, array('class' => 'webform'));
    drupal_set_title($node->title);
  }
  return $output;
}

/**
 * Paginate the webform report
 * @return a themed table with page links
 */
function _webform_report_pager($fields, $values, $node) {
  // for backward compatibility
  if(!$node->results_per_page) {
    $results_per_page = 20;
  } else {
    $results_per_page = $node->results_per_page;
  }
  // Add the css file for form display.
  drupal_add_css(drupal_get_path('module', 'webform_report') . '/webform_report.css');

  // break the array into chunks for pagination 
  $pages = array_chunk($values, $results_per_page, TRUE); 
  if(!$_GET['page']) {
    $_GET['page'] = '1';
  }
  $output = theme_table($fields, $pages[($_GET['page'] - 1)], array('class' => 'webform_report'));
  $output .= '<div class="pager">';
  if($_GET['page'] > 1) {
    $output .= '<a class="pager-first active" title="' . t('Go to first page') . '" href="?sort=' . $_GET['sort'] . '&order=' . $_GET['order'] . '&page=1">' . t('« first') .'</a>';
    $output .= '<a class="pager-first active" title="' . t('Go to previous page') . '" href="?sort=' . $_GET['sort'] . '&order=' . $_GET['order'] . '&page=' . ($_GET['page'] - 1) . '">' . t('‹ previous') .'</a>';
  }
  $output .= '<span class="pager-list">';
  $css_class = 'pager-next-active';
  foreach($pages as $key => $page) {
    switch($_GET['page']) {
      case($key + 1):
        $css_class = 'pager-current';
        break;
      case(1):
        $css_class = 'pager-first active';
        break;
      case(count($pages)):
        $css_class = 'pager-last active';
        break;
      default:
        $css_class = 'pager-next active';
        break;
    }
    if($css_class == 'pager-current') {
      $output .= '<strong class="' . $css_class . '">' . ($key + 1) . '</strong>';
    } else {
      $output .= '<a class="' . $css_class . '" href="?sort=' . $_GET['sort'] . '&order=' . $_GET['order'] . '&page=' .($key + 1) . '">' . ($key + 1) . '</a>';
    }
  }
  $output .= '</span>';
  if($_GET['page'] < count($pages)) {
    $output .= '<a class="pager-last active" title="' . t('Go to next page') . '" href="?sort=' . $_GET['sort'] . '&order=' . $_GET['order'] . '&page=' . ($_GET['page'] + 1) . '">' . t('next ›') .'</a>';
    $output .= '<a class="pager-last active" title="' . t('Go to last page') . '" href="?sort=' . $_GET['sort'] . '&order=' . $_GET['order'] . '&page=' . count($pages) . '">' . t('last »') .'</a>';
  }
  $output .= '</div>';
  return $output;
}

?>
