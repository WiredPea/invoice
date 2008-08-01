<?php
// $Id$

/**
 * @file
 * Invoice module
 * 
 * This module was developed by Platina Designs, http://www.platinadesigns.nl
 *
 * @author Pieter Vogelaar <ps.vogelaar@platinadesigns.nl>
 */

/**
 * Helper function to calculate invoice totals
 * 
 * The ROUND() function of MySQL is not very reliable, especially in MySQL 4.x,
 * that's why this function came into play
 * 
 * @param integer $invoice_number
 * @param integer $user_id
 * 
 * @return array $a_totals
 */
function _invoice_get_invoice_totals($invoice_number, $user_id=0) {
  $a_totals = array();
  $sql_user_addition = '';
  
  if ($user_id > 0) {
    $sql_user_addition = ' AND uid="'. intval($user_id) .'"';
  }
  
  $result = db_query("SELECT vat,quantity*unitcost as extotal,
    (quantity*unitcost)*((vat / 100) +1) as inctotal,
    (quantity*unitcost)*((vat / 100) +1) * (1 - (1 / ((vat / 100) +1))) as vattotal
    FROM {invoice_items}
    WHERE invoice_id=%d" . $sql_user_addition,
    $invoice_number
  );
  
  while ($row = db_fetch_object($result)) {
    $a_totals['extotal'] += _invoice_round($row->extotal, 2);
    $a_totals['inctotal'] += ($row->extotal * (1 + ($row->vat / 100)));
    $a_totals['vattotal'] += $row->vattotal;
  }

  $a_totals['extotal'] = _invoice_round($a_totals['extotal'], 2);
  $a_totals['inctotal'] = _invoice_round($a_totals['inctotal'], 2);
  $a_totals['vattotal'] = _invoice_round($a_totals['vattotal'], 2);
  
  return $a_totals;
}


/**
 * Helper function to easily get the name of the chosen template when adding an invoice
 */
function _invoice_get_chosen_template() {
  return !isset($_SESSION['invoice_template']) || empty($_SESSION['invoice_template']) ? variable_get('invoice_default_template', 'default') : $_SESSION['invoice_template'];
}

/**
 * Helper function to get the invoice in HTML format
 */
function _invoice_get_html($invoice_number, $type='print') {
  $nid = db_result(db_query("SELECT nid FROM {invoice_invoices} WHERE iid=%d", $invoice_number));
  $node = node_load($nid);
    
  $html = '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
  <html xmlns="http://www.w3.org/1999/xhtml" lang="nl" xml:lang="nl" dir="ltr">
  <head>
  <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-15" />
  ';
  $html = '<html><head>';
  $html .= '<link type="text/css" rel="stylesheet" media="all" href="http://'. $_SERVER['HTTP_HOST'] . base_path() . drupal_get_path('module', 'invoice') .'/templates/'. $node->invoice['template'] .'.css" />';
  $html .= '</head><body><div class="'. $type .'">';

  $html .= theme('invoice_body', $node, $type);

  // The eurosign is not supported as applicable character, so replace it with a ascii code
  $html = str_replace('€', '&#0128;', $html);
  
  $html .= '</div></body></html>';
  return $html;
}

/**
 * Helper function to get invoice id
 *
 * @return integer
 */
function _invoice_get_new_invoice_number($user_defined_invoice_number_check=FALSE) {
  if ($user_defined_invoice_number_check == TRUE) {
    $count = db_result(db_query("SELECT COUNT(*) FROM {invoice_invoices}"));
    if ($count == 0) {
      return 0;
    }
    else {
      return db_result(db_query("SELECT MAX(iid) FROM {invoice_invoices}"));
    }
  }
  
  $count = db_result(db_query("SELECT COUNT(*) FROM {invoice_invoices} WHERE iid LIKE '%". date("Ym") ."%'"));
  if ($count > 0) {
    $new_invoice_number = db_result(db_query("SELECT MAX(iid) FROM {invoice_invoices}")) + 1;
  }
  else {
    $new_invoice_number = date('Ym') .'001';
   }
 
  return $new_invoice_number;
}

/**
 * Helper function to add JS or CSS to Drupal
 */
function _invoice_add_css_js() {
  $a_invoice_js_settings = array(
    'host' => $_SERVER['HTTP_HOST'],
  );
  drupal_add_js(array('invoice' => $a_invoice_js_settings), 'setting');
  drupal_add_js(_invoice_get_js(), 'inline');
  drupal_add_css(drupal_get_path('module', 'invoice') .'/invoice.css');
}

/**
 * Helper function to get a rounded value
 *
 * Read the comments on http://www.php.net/round why the
 * standard PHP round() function doesn't work right.
 * 
 * For example with PHP round(38.675, 2) gives 38.67, but that must be 38.68
 * 
 * @param mixed $value
 * @param integer $precision
 * @return float
 */
function _invoice_round($value, $precision=0) {
  $rounded_value = round(round($value*pow(10, $precision+1), 0), -1)/pow(10, $precision+1);
  return $rounded_value;
}

/**
 * Helper function to get the available template names
 */
function _invoice_get_templates() {
  $a_templates = array('default');
  $files = file_scan_directory(dirname(__FILE__) .'/templates', '.inc');
  foreach ($files as $file) {
    if (!empty($file->name) && $file->name != 'default') {
      $a_templates[] = check_plain($file->name);
    }
  }
  
  return $a_templates;
}

/**
 * Helper function to get template variables
 */
function _invoice_get_variable($template, $name, $default=NULL) {
  static $a_templates = NULL;

  // Get all template settings from the database only one time
  if (!is_array($a_templates)) {
    $a_templates_names = _invoice_get_templates();
  
    $a_templates = array();
    $result = db_query("SELECT * FROM {invoice_templates} WHERE name IN ('". implode("','", $a_templates_names) ."')");
    while ($row = db_fetch_array($result)) {
      $a_templates[$row['name']] = $row;
    }
  }
  
  // Get template info that is stored in the database
  $a_template = $a_templates[$template];
  
  if (!empty($a_template[$name])) {
    return $a_template[$name];
  }
  else {
    if (!is_null($default)) {
      return $default;
    }
    else {
      // if $default is not null, check if a general/default value is available 
      return variable_get('invoice_'. $name, '');
    }
  }
}

/**
 * Helper function to get an icon
 */
function _invoice_get_icon($name, $url = NULL, $attributes = array(), $extension = 'png') {
  if (empty($attributes['alt'])) {
    $attributes['alt'] = $attributes['title'];
  }
  
  $img_addition = '';
  foreach ($attributes as $key => $value) {
    $img_addition .= ' '. $key .'="'. $value .'"';
  }
  
  $icon = '<img src="'. base_path() . drupal_get_path('module', 'invoice') .'/images/'. $name .'.'. $extension .'"'. $img_addition .' />';
  if (!empty($url)) {
    $icon = l($icon, $url, array('html' => TRUE));
    //$icon = '<a href="'. base_path() . $url .'" '. drupal_attributes($attributes) .'>'. $icon .'</a>';
  }
  
  return $icon;
}

/**
 * Helper function to collect all $_GET variables and put them in a string
 */
function _invoice_getvars_array_to_string($a_vars = array()) {

  unset($a_vars['q']);
  
  $s_vars = '?';
  foreach ($a_vars as $key => $value) {
    $s_vars .= $key .'='. $value .'&';
  }
  
  return rtrim($s_vars, '&');
}

/**
 * Helper function to convert the string with get variables to an array
 *
 * @param unknown_type $s_vars
 * @return unknown
 */
function _invoice_getvars_string_to_array($s_vars) {
  $exp = explode('&', $s_vars);
  $a_query_vars = array();
  // the first element is always "q", we don't want that element so start with $i=1 
  for ($i=1;$i<count($exp);$i++) {
    $sub_exp = explode('=', $exp[$i]);
    $a_query_vars[$sub_exp[0]] = $sub_exp[1];
  }
  return $a_query_vars;
}

/**
 * Helper function to include the dompdf library
 */
function invoice_dompdf_include_lib() {
  require_once(dirname(__FILE__) .'/dompdf/dompdf_config.inc.php');	
}
