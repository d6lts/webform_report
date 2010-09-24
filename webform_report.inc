<?php

// $Id$

/**
 * @file
 * This file contains common functions and functions required
 * to ouput a report for the webform report module
 */
 
/**
 * Search for nodes of type webform and returns their nids and titles
 * in an associative array.
 */
function _webform_report_get_webforms() {
  $webforms = array();
  $webform_types = variable_get('webform_node_types', array('webform'));
  $placeholders = db_placeholders($webform_types, 'varchar');
  $result = db_query(db_rewrite_sql("SELECT n.nid, n.title FROM {node} n WHERE n.type in ($placeholders)"), $webform_types);

  while ($row = db_fetch_object($result)) {
    $webforms[$row->nid] = t($row->title);
  }
  return $webforms;
}

/**
 * Get webform components.
 *
 * @param nid
 *   the node id of a webform
 * @return 
 *   an associative array of webform components
 */
function _webform_report_get_components($nid) {
  
  $components = array();
  
  // include meta components
  $components[-1] = array('name' => t('Submitted By User'), 'type' => 'string');
  $components[-2] = array('name' => t('Submission Date'), 'type' => 'date');
  $components[-3] = array('name' => t('Submission Time'), 'type' => 'time');
  $components[-4] = array('name' => t('Submission IP Address'), 'type' => 'string');
  $components[-5] = array('name' => t('Edit Link'), 'type' => 'link');
  $components[-6] = array('name' => t('View Link'), 'type' => 'link');

  // get components from webform  
  $result = db_query("SELECT c.cid, c.name, c.type FROM {webform_component} c WHERE c.type <> 'fieldset' AND c.nid = %d ORDER BY c.weight", $nid);
  while ($row = db_fetch_object($result)) {
    $components[$row->cid] = array('name' => filter_xss($row->name), 'type' => $row->type);
  }
  
  return $components;
}

/**
 * return an associative array containing the webform report columns
 *
 * @param node
 *   the webform report node
 * @param components
 *   array of webform components
 * @return 
 *   an associative array of webform report columns
 */
function _webform_report_get_columns($node, $components) {

  // build list of columns on the report
  $columns = array();
  if (is_array($node->columns)) {
    for ($i=0; $i<count($node->columns); $i++) {
      $cid = $node->columns[$i]['cid'];
      if (array_key_exists($cid, $components)) {
        $columns[] = array(
          'name' => $components[$cid]['name'],
          'type' => $components[$cid]['type'],
          'format' => $node->columns[$i]['format'],
          'cid' => $node->columns[$i]['cid'],
        );
      }
    }
  }
  
  return $columns;
}

/**
 * return an associative array containing the webform report filters
 *
 * @param node
 *   the webform report node
 * @param components
 *   array of webform components
 * @return 
 *   an associative array of webform report filters
 */
function _webform_report_get_filters($node, $components) {

  // build list of filters
  $filters = array();
  if (is_array($node->filters)) {
    for ($i=0; $i<count($node->filters); $i++) {
      $cid = $node->filters[$i]['field'];
      if (array_key_exists($cid, $components)) {
        $filters[] = array(
          'name' => $components[$cid]['name'],
          'type' => $components[$cid]['type'],
          'ftype' => $node->filters[$i]['type'],
          'value' => $node->filters[$i]['value'],
          'cid' => $node->filters[$i]['field'],
        );
      }
    }
  }
  
  return $filters;
}

/**
 * return an associative array containing the webform report sort criteria
 *
 * @param node
 *   the webform report node
 * @param components
 *   array of webform components
 * @return 
 *   an associative array of webform report sort criteria
 */
function _webform_report_get_sorton($node, $components) {

  // build list of sort parameters
  $sorton = array();
  if (is_array($node->sorton)) {
    for ($i=0; $i<count($node->sorton); $i++) {
      $cid = $node->sorton[$i]['cid'];
      if (array_key_exists($cid, $components)) {
        $sorton[] = array(
          'name' => $components[$cid]['name'],
          'type' => $components[$cid]['type'],
          'order' => $node->sorton[$i]['order'],
          'cid' => $node->sorton[$i]['cid'],
        );
      }
    }
  }

  return $sorton;
}

/**
 * Get submission data for the specified webform.
 *
 * @param node 
 *   the current node object
 * @return 
 *   a database query result set
 */
function _webform_report_get_data($node) {

  if (isset($node->wnid)) {
    return db_query("
      SELECT w.nid, c.name, c.cid, d.nid, d.sid, d.data, s.uid, u.name as user, s.submitted, s.remote_addr
      FROM {webform} w
      LEFT JOIN {webform_submitted_data} d ON w.nid = d.nid
      LEFT JOIN {webform_component} c ON d.cid = c.cid
      LEFT JOIN {webform_submissions} s  ON d.sid = s.sid
      LEFT JOIN {users} u ON s.uid = u.uid
      WHERE d.nid = c.nid 
      AND c.nid = s.nid
      AND s.nid = %d
      ORDER BY d.sid, c.cid, d.no", $node->wnid);
  }
  else {
    return NULL;
  }
}

/**
 * Get node body content for the specified webform report.
 *
 * @param node 
 *   the current node object
 * @param formatcsv 
 *   if TRUE format the output as a CSV file
 * @return 
 *   a string of text or a themed table
 */
function _webform_report_get_body_content($node, $formatcsv = FALSE) {

  $output = '';
  
  // query submissions  
  $rs = _webform_report_get_data($node);  

  // if any submissions
  if (!empty($rs)) {
  
    // get report criteria
    $components = _webform_report_get_components($node->wnid);
    $columns = _webform_report_get_columns($node, $components);
    
    // if any columns
    if (count($columns) > 0) {
    
      // get other report criteria
      $filters = _webform_report_get_filters($node, $components);
      $sorton = _webform_report_get_sorton($node, $components);
  
      // init values
      $headers = array();
      $rows = array();
      $fields = array();
      $csid = 0;
      
      // set column headers
      foreach ($columns as $index => $col) {
        // set report header - also save type for later
        $headers[] = array(
          'data' => $col['name'],
          'field' => $col['cid'],
          'type' => $col['type']
        );
        // fields by cid for quick lookup
        $fields[$col['cid']] = $col['name'];
      }
      // add filter fields to lookup
      foreach ($filters as $index => $filter) {
        // fields by cid for quick lookup
        $fields[$filter['cid']] = $filter['name'];
      }
      
      // get column data
      while (TRUE) {
        
        // get next submission data
        $row = db_fetch_object($rs);
      
        // check for end of data or end of submission
        if (!$row || $row->sid != $csid) {
          
          // process data for this submission, if any
          if ($csid != 0) {
          
            // test submission against filters
            if (_webform_report_test_filters($data, $filters)) {
              // output row if filters pass
              $rows[] = _webform_report_output($data, $columns);
            }
          }
          // if end of submission data, exit
          if (!$row) {
            break;
          }
          
          // set current submission id
          $csid = $row->sid;
          
          // empty raw data row
          $data = array();
            
          // fill in meta fields
          if (array_key_exists(-1, $fields)) {
            $data[-1] = array(l($row->user, 'user/' . $row->uid));
          }
          if (array_key_exists(-2, $fields)) {
            $data[-2] = array($row->submitted);
          }
          if (array_key_exists(-3, $fields)) {
            $data[-3] = array($row->submitted);
          }
          if (array_key_exists(-4, $fields)) {
            $data[-4] = array($row->remote_addr);
          }
          if (array_key_exists(-5, $fields)) {
            $data[-5] = array(l('edit', 'node/'. $row->nid . '/submission/' . $row->sid . '/edit'));
          }
          if (array_key_exists(-6, $fields)) {
            $data[-6] = array(l('view', 'node/'. $row->nid . '/submission/' . $row->sid));
          }
        }
      
        // if component is on report
        if (array_key_exists($row->cid, $fields)) {
          // add raw data
          $data[$row->cid][] = $row->data;
        }
        
      }   // end - while (TRUE)...
      
      // see if any rows are available
      if (count($rows) > 0) {
        // sort
        _webform_report_sort($headers, $rows, $sorton);
        // output in requested format  
        if ($formatcsv) {
          // format as csv
          $output = _webform_report_output_csv($headers, $rows);
        }
        else {
          // Display number of rows after description.
          $output .= filter_xss_admin($node->description) . 
                     ' (' . count($rows) . ' ' . t('results') . ') ';
          if (!$node->options['hide_csv']) {
            $output .= l(t('Download as CSV'), 'node/' . arg(1) . '/csv') . '</p>';
          }
          // output current page
          $output .= _webform_report_pager($headers, $rows, $node);
        }
      }
      // no submissions met criteria
      else {
        $output = t('There are no submissions that match the criteria for the selected webform.');
      }
      
    }   // end - if (count($columns) > 0)...
    
    // webform report does not have any criteria
    else {
      $output = t('It appears that no criteria have been specified for this report. 
        Please click on the Criteria tab to add webform data to your report.');
    }
    
  }   // end - if (!empty($rs))...
  
  // the selected webform has no submissions.
  else {
    $output = t('There are no submissions for the selected webform. Either the form
       has not yet been completed by anyone, or the results have been cleared. This will not
       prevent you from creating this report, but this message will be displayed on the report
       page until someone submits the selected webform.');
  }
  
  return $output;
}

/**
 * test data against filters
 *
 * @param data
 *   array of raw submission data
 * @param filters
 *   list of filters to test against data
 * @return 
 *   TRUE if filters passed, otherwise FALSE
 */
function _webform_report_test_filters($data, $filters) {

  // filter result, return true if no filters
  $ok = TRUE;

  // if any filters
  if (count($filters) > 0) {
  
    // loop through all filters  
    foreach($filters as $index => $filter) {

      // check that cid is in data
      if (array_key_exists($filter['cid'], $data)) {
    
        // reset result for each filter  
        $ok = FALSE;
        
        // get raw data
        $raw = $data[$filter['cid']];
        
        // format value
        $value = _webform_report_format_data($raw, $filter['cid'], $filter['type']);
        
        // prepare filter values
        $filter_data = strip_tags(strtolower(trim($value['data'])));
        $filter_value = strtolower(trim($filter['value']));
        
        // apply filter
        switch($filter['ftype']) {
    
          // none
          case 0:
            $ok = TRUE;
            
          // Begins with x.
          case 1:
            if (substr($filter_data, 0, drupal_strlen($filter_value)) == $filter_value) {
              $ok = TRUE;
            }
            break;
        
          // Does not begin with x.
          case 2:
            if (substr($filter_data, 0, drupal_strlen($filter_value)) != $filter_value) {
              $ok = TRUE;
            }
            break;
            
          // Contains x.
          case 3:
            if (strpos($filter_data, $filter_value) !== FALSE) {
              $ok = TRUE;
            }
            break;
            
          // Does not contain x.
          case 4:
            if (strpos($filter_data, $filter_value) === FALSE) {
              $ok = TRUE;
            }
            break;
            
          // Equals x.
          case 5:
            if ($filter_data == $filter_value) {
              $ok = TRUE;
            }
            break;
            
          // Does not equal x.
          case 6:
            if ($filter_data != $filter_value) {
              $ok = TRUE;
            }
            break;
            
          // is empty
          case 7: 
            if (drupal_strlen($filter_data) == 0) {
              $ok = TRUE;
            }
            break;
            
          // is not empty        
          case 8: 
            if (drupal_strlen($filter_data) != 0) {
              $ok = TRUE;
            }
            break;
            
          // greater than
          case 9: 
            if ($filter_data > $filter_value) {
              $ok = TRUE;
            }
            break;
            
          // less than
          case 10: 
            if ($filter_data < $filter_value) {
              $ok = TRUE;
            }
            break;
          }   // end - switch($filter['type'])...
          
        }   // end - if (array_key_exists($filter['cid'], $data)...
        
        // if filter did not pass, don't check any further
        if (!$ok) {
          break;
        }
      
      }   // end - foreach($filters as $index => $filter)...
      
    }   // end - if (count($filters) > 0)...
  
  // return filter result
  return $ok;
}

/**
 * format output table row for report
 *
 * @param data 
 *   raw submission data - array with key of cid, value an array of raw data
 * @param columns
 *   list of report columns
 * @return 
 *   table row suitable for theme_table
 */
 function _webform_report_output($data, $columns) {

  $row = array();

  // add each column to output row, applying formatting
  foreach ($columns as $index => $col) {

    // check if data exists
    if (array_key_exists($col['cid'], $data)) {
    
      // get raw data
      $raw = $data[$col['cid']];

      // add column to output row
      $row[] = _webform_report_format_data($raw, $col['cid'], $col['type'], $col['format']);
    }
    else {
    
      // no data, add dummy data
      $row[] = array('data' => '', 'field' => $col['cid']);
    }
  }
  
  return $row;
}

/**
 * format raw submission data
 *
 * @param raw
 *   array of raw submission data
 * @param cid
 *   webform componenet id of data
 * @param type
 *   webform type of data
 * @param format
 *   desired data output format
 * @return 
 *   array of formatted table data
 */
function _webform_report_format_data($raw, $cid, $type, $format = '') {

  $out = '';
  
  // handle file type
  if ($type == 'file') {
    $tmp = unserialize($raw[0]);
    $link = '&nbsp;';
    if (!empty($tmp['filename'])) {
      $link = '<a href="' . base_path() . $tmp['filepath'] . '">' . 
              $tmp['filename'] . ' (' . (int) ($tmp['filesize'] / 1024) .' KB)' . 
              '</a>';
    }
    $out = array('data' => $link, 'field' => $cid, 'sort' => $tmp['filepath']);
  }
    
  // handle dates
  else if ($type == 'date') {
    $ts = 0;
    // if report date
    if ($cid > 0) {
      // if date was selected
      if ($raw[0] && $raw[1]) {
        // make timestamp, dates are stored month, day, year
        $ts = strtotime($raw[0] .'/'. $raw[1] .'/'. $raw[2]);
      }
    }
    // otherwise get submission timestamp
    else {
      $ts = $raw[0];
    }
    // if valid timestamp
    if ($ts > 0) {
      // set default format if none given
      if (empty($format)) {
        $format = 'Y-m-d';
      }
      // format the date
      $out = array('data' => date($format, $ts), 'field' => $cid, 'sort' => $ts);
    }
  }
    
  // handle times
  else if ($type == 'time') {
    $ts = 0;
    if ($cid > 0) {
      // if time was selected
      if ($raw[0] && $raw[1]) {
        // make timestamp, times are stored hh, mm, ampm
        $ts = strtotime($raw[0] . ':' . $raw[1] . $raw[2]);
      }
    }
    else {
      // otherwise get submission timestamp
      $ts = $raw[0];
    }
    // if valid timestamp
    if ($ts > 0) {
      // default format if none given
      if (empty($format)) {
        $format = 'H:i';
      }
      // format the time
      $out = array('data' => date($format, $ts), 'field' => $cid, 'sort' => $ts);
    }
  }
  
  // handle other column types
  else {
    // get raw data
    $tmp = filter_xss_admin(implode(',', $raw));
    // build sort key  
    $sort = strip_tags($tmp);
    // allow for accents if module is present
    if (function_exists('_accents_remove_accents')) {
      $sort = _accents_remove_accents($sort);
    }
    $out = array('data' => $tmp, 'field' => $cid, 'sort' => $sort);

    // add links
    if (valid_email_address($out['data'])) {
      $out['data'] = l($out['data'], 'mailto:' . $out['data']);
    }
    else {
      if (valid_url($out['data'], TRUE)) {
          $out['data'] = l($out['data'], $out['data']);
      }
    }
  }
    
  // return formatted data
  return $out;
}

/**
 * sort row data
 *
 * anonymous sort comparison function creation adapted from:
 * http://www.the-art-of-web.com/php/sortarray/
 *
 * @param headers
 *   array of report headers
 * @param rows
 *   array of report rows
 * @param sorton
 *   list of default sort parameters
 */
function _webform_report_sort(&$headers, &$rows, $sorton) {

  // will be empty if user has not clicked on a header
  $init = !isset($_GET['order']);
  
  // get current sort - 'name' key is column name, 'sql' is cid
  $ts = tablesort_get_order($headers);
  // get sort order
  $ts_order = tablesort_get_sort($headers);

  // if inital display (no column header has been clicked)
  if ($init) {
    // and no sort parameters
    if (count($sorton) == 0) {
      // we must sort on first column, that is what theme_table defaults to
      $headers[0]['sort'] = 'asc';
      $code = 'return ' . _webform_report_sort_column(0, FALSE, $headers[0]['type']) . ';';
    }
    // otherwise setup initial sort, if specified
    else {
      $a = array();

      // loop through sorts      
      foreach ($sorton as $index => $sort) {
        // find the header
        foreach ($headers as $hindex => $header) {
            
          if ($header['field'] == $sort['cid']) {

            // mark the first sort header
            if (count($a) == 0) {
              if ($sort['order'] == SORT_ASC) {
                $headers[$hindex]['sort'] = 'asc';
              }
              else {
                $headers[$hindex]['sort'] = 'desc';
              }
            }   // end - if (count($a) == 0)...
              
            // add the sort function
            $a[] = _webform_report_sort_column($hindex, ($sort['order'] == SORT_DESC), $sort['type']);
            break;
            
          }  // end - if ($header['field'])...
        }   // end - foreach ($headers as...
      }   // end - foreach ($sorton...
      
      // format the complete sort function
      if (count($a) == 0) {
        // probably no columns
        $code = 'return 0;';
      }
      else if (count($a) == 1) {
        // only one clause, return it
        $code = 'return ' . $a[0] . ';';
      }
      else {
        // multiple clauses, build descending compare function
        $code = '';
        for ($i=0; $i<count($a); $i++) {
          if ($i < count($a) - 1) {
            $code .= 'if (($tmp = ' . $a[0] . ') == 0) { ';
          }
          else {
            $code .= '$tmp = ' . $a[$i] . '; ';
          }
        }
        for ($i=0; $i<count($a) - 1; $i++) {
          $code .= ' } ';
        }
        $code .= 'return $tmp;';
      }
    }
  }
  // user has clicked on a column header - must sort that column
  else {
    foreach ($headers as $hindex => $header) {
      if ($header['field'] == $ts['sql']) {
        $headers[$hindex]['sort'] = $ts_order;
        $code = 'return ' . _webform_report_sort_column($hindex, ($ts_order == 'desc'), $header['type']) . ';';
        break;
      }
    }
  }
  // sort the rows
  usort($rows, create_function('$a,$b', $code));
}

/**
 * Generate a sort function for a column
 *
 * @param index
 *   the index of the field to sort
 * @param desc 
 *   if true sort is descending
 * @param type
 *   the field type 
 * @return 
 *   the sort function for the given column
 */
function _webform_report_sort_column($index, $desc = FALSE, $type = '') {

  // create comparison based on type
  // for dates and times compare timestamps
  if ($type == 'date' || $type == 'time') {
      $code = "(\$a[$index]['sort']-\$b[$index]['sort'])";
  }
  // for all other types, handle as string
  else {
    $code = "strcasecmp(\$a[$index]['sort'], \$b[$index]['sort'])";
  }
  // invert output for descending sort
  if ($desc) {
    $code = $code . ' * -1';
  }
  $code = '(' . $code . ')';

  return $code;
}

/**
 * Display a list of webform reports in the admin area.
 *
 * @return 
 *   a themed table
 */
function _webform_report_page() {

  $output = NULL;
  if (user_access('access webform reports')) {
    $headers = array(t('View'), t('Edit'), t('Delete'));

    $result = db_query(db_rewrite_sql("SELECT n.nid, n.title FROM {node} n WHERE n.type='webform_report'"));
   
    while ($node = db_fetch_object($result)) {
      $rows[] = array(
        l($node->title,'node/' . $node->nid),
        l(t('edit'), 'node/'.$node->nid.'/edit'),
        l(t('delete'), 'node/'.$node->nid.'/delete')
      );
    }
  
    $output = theme_table($headers, $rows, array('class' => 'webform'));
  }
  return $output;
}

/**
 * Paginate the webform report.
 *
 * @return 
 *   a themed table with page links
 */
function _webform_report_pager($headers, $rows, $node) {

  global $pager_page_array, $pager_total;

  // get results per page setting - start with default
  $results_per_page = 20;
  // if value set, use that
  if (array_key_exists('results_per_page', $node->options)) { 
    $results_per_page = $node->options['results_per_page'];
  }
  // Add the css file for form display.
  drupal_add_css(drupal_get_path('module', 'webform_report') . '/webform_report.css');

  // Break the array into chunks for pagination.
  if ($results_per_page != 0) {
    $pages = array_chunk($rows, $results_per_page, TRUE); 
  }
  else {
    $pages[0] = $rows;
  }
  
  // Grab the 'page' query parameter.
  // Taken from pager_query() in pager.inc
  $page = isset($_GET['page']) ? $_GET['page'] : '';
  
  // Convert comma-separated $page to an array, used by other functions.
  // Taken from pager_query() in pager.inc
  $pager_page_array = explode(',', $page);
    
  // format the table with the current page
  if ($page == '') $page = 0;
  $output = theme_table($headers, $pages[$page], array('class' => 'webform_report'));
    
  // Put some magic in the two global variables
  // Based on code in pager_query() in pager.inc
  $pager_total[0] = count($pages);
  $pager_page_array[0] =
    max(0, min(
      (int)$pager_page_array[0],
      ((int)$pager_total[0]) - 1)
    );
      
  // Add the pager to the output.
  $output .= theme('pager', NULL, $results_per_page, 0);

  return $output;
}

/**
 * Output a webform report in CSV format
 *
 * @param node
 *   the current node
 */
function webform_report_csv($node) {

  $output = _webform_report_get_body_content($node, TRUE);
      
  $fname = 'wfr_export.csv';
  header('Content-Type: text/plain');
  header('Content-Length: ' . strlen($output));
  header('Content-Disposition: attachment; filename="' . $fname . '"');
  echo $output;
}

/**
 * Format webform report data as a CSV
 *
 * @param headers
 *   report headers
 * @param rows
 *   report rows
 * @return 
 *   CSV output
 */
function _webform_report_output_csv($headers, $rows) {

  $output = '';
  
  // output headers
  $tmp = array();
  foreach($headers as $header) {
    $tmp[] .= _webform_report_format_csv_column($header['data']);
  }
  $output .= implode(',', $tmp) . "\n";

  // output rows
  foreach($rows as $row) {
    $tmp = array();
    foreach($row as $cell) {
      $tmp[] = _webform_report_format_csv_column(strip_tags($cell['data']));
    }
    $output .= implode(',', $tmp) . "\n";
  }

  return $output;
}

/**
 * Format a CSV column value
 *
 * @param value
 *   the value to format for CSV
 * @return 
 *   CSV column
 */
function _webform_report_format_csv_column($value) {

  // if value contains a comma, it should be delimited by quotes
  if (strpos($value, ',')) {
      // if value contains quotes, double the quotes
      if (strpos($value, '"')) {
        return '"' . str_replace('"', '""', $value) . '"';
      }
      else {
        return '"' . $value . '"';
      }
  }
  return $value;
}
