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
 * Implementatin of node_form()
 */
function invoice_form(&$form_state, $node) {
  _invoice_add_css_js();
  
  $form = array();
  $form['invoice_template'] = array(
    '#type' => 'fieldset',
    '#title' => t('Invoice template'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#weight' => 1,    
  );
  
  // Get template names
  $a_templates = _invoice_get_templates();
  
  // Build array for selecting the default template
  $a_template_options = array();
  foreach ($a_templates as $s_template) {
    $a_template_options[$s_template] = ucfirst($s_template);
  }
  
  $active_template = empty($form_state->invoice['template']) ? _invoice_get_chosen_template() : $form_state->invoice['template'];
    
  $form['invoice_template']['template'] = array(
    '#type' => 'select',
    '#title' => '',
    '#options' => $a_template_options,
    '#default_value' => $active_template,
    '#attributes' => empty($form_state->invoice['template']) ? array('onchange' => 'invoice_set_template(this.value)') : array(),
    '#description' => t("When editing this invoice, you'll have to save first before you can see template changes."),
  );
  
  if (empty($form_state->invoice['template'])) {
    $_SESSION['invoice_template'] = _invoice_get_chosen_template();
  }
  
  $form['customer'] = array(
    '#type' => 'fieldset',
    '#title' => t('Customer details'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t('You either have to fill in Company name or Lastname. Firstname is optional and is only saved if a lastname is filled in.'),
    '#weight' => 2,
  );
  $form['customer']['search'] = array(
    '#type' => 'textfield',
    '#title' => t('Search customer'),
    '#maxlength' => 60,
    '#autocomplete_path' => 'invoice/search/customer',
    '#default_value' => !empty($form_state->customer['company_name']) ? $form_state->customer['company_name'] : $form_state->customer['lastname'] . (!empty($form_state->customer['firstname']) ? ', '. $form_state->customer['firstname'] : ''),
  );
  $form['customer']['company_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Company name'),
    '#required' => FALSE,
    '#default_value' => $form_state->customer['company_name'],
  );
  $form['customer']['firstname'] = array(
    '#type' => 'textfield',
    '#title' => t('Firstname'),
    '#required' => FALSE,
    '#default_value' => $form_state->customer['firstname'],
  );
  $form['customer']['lastname'] = array(
    '#type' => 'textfield',
    '#title' => t('Lastname'),
    '#required' => FALSE,
    '#default_value' => $form_state->customer['lastname'],
  );
    
  $invoice_items_table_header = array(
    t('Description'),
    t('VAT'),
    t('Count'),
    t('Unitcost (ex. VAT)'),
    t('Unitcost (inc. VAT)'),
    t('Subtotal (ex. VAT)'),
    t('Subtotal (ex. VAT'),
    '',
  );
  
  $form['invoice_items'] = array(
    '#type' => 'fieldset',
    '#title' => t('Invoice items'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#weight' => 4,
  );

  // Set locale so money has the right format for the preferred culture
  if (intval($form_state->invoice['invoice_number']) == 0) {
    if ($locale = _invoice_get_variable(_invoice_get_chosen_template(), 'locale')) {
      setlocale(LC_MONETARY, $locale);
    }
  }
  elseif ($locale = _invoice_get_variable($active_template, 'locale')) {
    setlocale(LC_MONETARY, $locale);
  }
  
  // Get invoice items
   $a_items = _invoice_get_invoice_items($form_state->invoice['invoice_number']);
  $count = $a_items['count'];
  $invoice_items_table_rows = $a_items['rows'];

  // If now rows are found add an empty row
  if ($count == 0) {
    $invoice_items_table_rows = array(
      array('data' => array(array('data' => t('Empty') .'...', 'colspan' => '7')), 'class' => 'invoice-items-empty'),    
    );
  }
  else {
    // Count the added items
    $a_totals = _invoice_get_invoice_totals($form_state->invoice['invoice_number'], $GLOBALS['user']->uid);
    $total->extotal = $a_totals['extotal'];
    $total->inctotal = $a_totals['inctotal'];
  }
  
  $invoice_items_table = theme('invoice_table', $invoice_items_table_header, $invoice_items_table_rows, array('disable_sticky_header' => TRUE));
  $invoice_items_table_footer = '<tfoot><tr><td colspan="5"></td><td class="extotal">'. money_format('%.2n', _invoice_round($total->extotal, 2)) .'</td><td class="inctotal">'. money_format('%.2n', _invoice_round($total->inctotal, 2)) .'</td></tr></tfoot>';
  $invoice_items_table = str_replace('</table>', $invoice_items_table_footer .'</table>', $invoice_items_table);
  
  $form['invoice_items']['items'] = array(
    '#value' => '<div class="invoice-items">'. $invoice_items_table .'</div>',    
  );
  $form['invoice_items']['iid'] = array(
    '#type' => 'hidden',
    '#title' => t('Invoice item id'),
    '#required' => FALSE,
  );
  $form['invoice_items']['description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description'),
    '#required' => FALSE,
    '#rows' => 1,
  );  
  $form['invoice_items']['quantity'] = array(
    '#type' => 'textfield',
    '#title' => t('Quantity'),
    '#required' => FALSE,
    '#size' => 5,
  );
  $form['invoice_items']['price_without_vat'] = array(
    '#type' => 'textfield',
    '#title' => t('Price without VAT'),
    '#required' => FALSE,
    '#description' => t("If you don't fill in this field, you'll have to fill in \"Price with VAT\""),
  );
  $form['invoice_items']['price_with_vat'] = array(
    '#type' => 'textfield',
    '#title' => t('Price with VAT'),
    '#required' => FALSE,
    '#description' => t("If you don't fill in this field, you'll have to fill in \"Price without VAT\""),
  );
  $form['invoice_items']['vat'] = array(
    '#type' => 'textfield',
    '#title' => t('VAT percentage'),
    '#required' => TRUE,
    '#attributes' => array('style' => 'width:40px;'), // size attribute didn't work here for some strange reason
    '#default_value' => _invoice_get_variable($active_template, 'vat'),
  );
  $form['invoice_items']['save_item'] = array(
    '#value' => '<input type="button" id="button-save-item" name="button_save_item" value="'. t('Add item') .'" onclick="invoice_save_item()" />',    
  );
  
  $form['customer_optional'] = array(
    '#type' => 'fieldset',
    '#title' => t('Customer details') .' ('. t('optional') .')',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 3, 
  );
  $form['customer_optional']['street'] = array(
    '#type' => 'textfield',
    '#title' => t('Street'),
    '#required' => FALSE,
    '#default_value' => $form_state->customer['street'],
  );
  $form['customer_optional']['building_number'] = array(
    '#type' => 'textfield',
    '#title' => t('Building number'),
    '#required' => FALSE,
    '#default_value' => $form_state->customer['building_number'],
  );
  $form['customer_optional']['zipcode'] = array(
    '#type' => 'textfield',
    '#title' => t('Zipcode'),
    '#required' => FALSE,
    '#default_value' => $form_state->customer['zipcode'],
  );
  $form['customer_optional']['city'] = array(
    '#type' => 'textfield',
    '#title' => t('City'),
    '#required' => FALSE,
    '#default_value' => $form_state->customer['city'],
  );
  $form['customer_optional']['country'] = array(
    '#type' => 'textfield',
    '#title' => t('Country'),
    '#required' => FALSE,
    '#default_value' => $form_state->customer['country'],
  );
  $form['customer_optional']['coc_number'] = array(
    '#type' => 'textfield',
    '#title' => t('Chamber of Commerce number'),
    '#required' => FALSE,
    '#default_value' => $form_state->customer['coc_number'],
  );
  $form['customer_optional']['vat_number'] = array(
    '#type' => 'textfield',
    '#title' => t('VAT number'),
    '#required' => FALSE,
    '#default_value' => $form_state->customer['vat_number'],
  );
  $form['customer_optional']['customer_description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description'),
    '#required' => FALSE,
    '#default_value' => $form_state->customer['description'],
  );
  
  $form['invoice_details'] = array(
    '#type' => 'fieldset',
    '#title' => t('Invoice details') .' ('. t('optional') .')',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 5,
  );
  
  // Only display this form field when creating a node
  if (empty($form_state->invoice['invoice_number'])) {
    $form['invoice_details']['user_defined_invoice_number'] = array(
      '#type' => 'textfield',
      '#title' => t('User defined invoice number'),
      '#required' => FALSE,
      '#default_value' => '',
      '#attributes' => array('style' => 'width:200px;'), // size attribute didn't work here for some strange reason
      '#description' => t('You can define an invoice number here. The number has to be higher than the latest invoice number though. It also has to be a number.'),
    );
  }
  $form['invoice_details']['invoice_number'] = array(
    '#type' => 'hidden',
    '#title' => t('Invoice number'),
    '#required' => FALSE,
    '#default_value' => $form_state->invoice['invoice_number'],
  );
  $form['invoice_details']['pay_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Pay limit'),
    '#required' => FALSE,
    '#default_value' => empty($form_state->invoice['pay_limit']) ? _invoice_get_variable($active_template, 'pay_limit') : $form_state->invoice['pay_limit'],
    '#description' => t('Pay limit in days'),
    '#attributes' => array('style' => 'width:40px;'), // size attribute didn't work here for some strange reason
  );
  $form['invoice_details']['invoice_description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description'),
    '#required' => FALSE,
    '#default_value' => $form_state->invoice['description'],
  );

  return $form;
}

/**
 * Invoice settings form
 */
function invoice_settings_form() {
    
  // Get template names
  $a_templates = _invoice_get_templates();
  
  $form['general'] = array(
    '#type' => 'fieldset',
    '#title' => t('General settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,    
  );
  $form['general']['locale'] = array(
    '#type' => 'textfield',
    '#title' => t('Locale'),
    '#required' => TRUE,
    '#default_value' => variable_get('invoice_locale', ''),
    '#description' => t('Category/locale names can be found in !rfc1766 and !iso639. Different systems have different naming schemes for locales.',
      array(
       '!rfc1766' => l('» RFC 1766', 'http://www.faqs.org/rfcs/rfc1766', array('absolute' => TRUE)),
       '!iso639' => l('» ISO 639', 'http://www.w3.org/WAI/ER/IG/ert/iso639.htm', array('absolute' => TRUE)),
      )
    ),
    '#size' => 8,
  );
  $form['general']['date_format'] = array(
    '#type' => 'textfield',
    '#title' => t('Date format'),
    '#required' => TRUE,
    '#default_value' => variable_get('invoice_date_format', ''),
    '#description' => t('The date on the invoice will look like: '. date(variable_get('invoice_date_format', ''))),
    '#size' => 20,
  );
  $form['general']['vat'] = array(
    '#type' => 'textfield',
    '#title' => t('Default VAT precentage'),
    '#required' => TRUE,
    '#default_value' => variable_get('invoice_vat', ''),
    '#size' => 3,
  );
  $form['general']['pay_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Pay limit'),
    '#required' => TRUE,
    '#default_value' => variable_get('invoice_pay_limit', ''),
    '#description' => t('Pay limit in days'),
    '#size' => 3,
  );
  $form['general']['supplier'] = array(
    '#type' => 'fieldset',
    '#title' => t('Supplier settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['general']['supplier']['company_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Company name'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_company_name', ''),    
  );
  $form['general']['supplier']['street'] = array(
    '#type' => 'textfield',
    '#title' => t('Street'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_street', ''),    
  );
  $form['general']['supplier']['building_number'] = array(
    '#type' => 'textfield',
    '#title' => t('Building number'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_building_number', ''),    
  );
  $form['general']['supplier']['zipcode'] = array(
    '#type' => 'textfield',
    '#title' => t('Zipcode'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_zipcode', ''),    
  );
  $form['general']['supplier']['city'] = array(
    '#type' => 'textfield',
    '#title' => t('City'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_city', ''),    
  );
  $form['general']['supplier']['country'] = array(
    '#type' => 'textfield',
    '#title' => t('Country'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_country', ''),    
  );
  $form['general']['supplier']['phone'] = array(
    '#type' => 'textfield',
    '#title' => t('Phone'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_phone', ''),    
  );  
  $form['general']['supplier']['fax'] = array(
    '#type' => 'textfield',
    '#title' => t('Fax'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_fax', ''),    
  );
  $form['general']['supplier']['email'] = array(
    '#type' => 'textfield',
    '#title' => t('Email'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_email', ''),    
  );
  $form['general']['supplier']['web'] = array(
    '#type' => 'textfield',
    '#title' => t('Web address'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_web', ''),    
  );
  $form['general']['supplier']['coc_number'] = array(
    '#type' => 'textfield',
    '#title' => t('CoC Number'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_coc_number', ''),    
  );
  $form['general']['supplier']['vat_number'] = array(
    '#type' => 'textfield',
    '#title' => t('VAT Number'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_vat_number', ''),    
  );  
  
  // Build array for selecting the default template
  $a_default_template = array();
  foreach ($a_templates as $s_template) {
    $a_default_template[$s_template] = ucfirst($s_template);
  }
  
  $form['general']['default_template'] = array(
    '#type' => 'select',
    '#title' => t('Default template'),
    '#options' => $a_default_template,
    '#default_value' => variable_get('invoice_default_template', 'default'),
    '#required' => TRUE,
    '#size' => 1,
  );

  // Build form for template values
  foreach ($a_templates as $s_template) {
    $form[$s_template] = array(
      '#type' => 'fieldset',
      '#title' => t('Template') .' ('. $s_template .')',
      '#collapsible' => TRUE,
      '#collapsed' => $s_template == 'default' ? FALSE : TRUE,
      '#description' => t('If fields are also set in invoice general settings and the template field is empty, the general setting of the field will be used.'),
    );
    $form[$s_template][$s_template .'_locale'] = array(
      '#type' => 'textfield',
      '#title' => t('Locale'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($s_template, 'locale', ''),
      '#description' => t('Category/locale names can be found in !rfc1766 and !iso639. Different systems have different naming schemes for locales.',
        array(
         '!rfc1766' => l('» RFC 1766', 'http://www.faqs.org/rfcs/rfc1766', array('absolute' => TRUE)),
         '!iso639' => l('» ISO 639', 'http://www.w3.org/WAI/ER/IG/ert/iso639.htm', array('absolute' => TRUE)),
        )
      ),
      '#size' => 20,
    );
    $form[$s_template][$s_template .'_date_format'] = array(
      '#type' => 'textfield',
      '#title' => t('Date format'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($s_template, 'date_format', ''),
      '#description' => t('The date on the invoice will look like: '. date(_invoice_get_variable($s_template, 'date_format'))),
      '#size' => 20,
    );
    $form[$s_template][$s_template .'_vat'] = array(
      '#type' => 'textfield',
      '#title' => t('Default vat percentage'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($s_template, 'vat', ''),
      '#size' => 3,
    );
    $form[$s_template][$s_template .'_pay_limit'] = array(
      '#type' => 'textfield',
      '#title' => t('Pay limit'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($s_template, 'pay_limit', ''),
      '#description' => t('Pay limit in days'),
      '#size' => 3,
    );
    $form[$s_template][$s_template .'_supplier'] = array(
      '#type' => 'fieldset',
      '#title' => t('Supplier settings'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form[$s_template][$s_template .'_supplier'][$s_template .'_supplier_company_name'] = array(
      '#type' => 'textfield',
      '#title' => t('Company name'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($s_template, 'supplier_company_name', ''),    
    );
    $form[$s_template][$s_template .'_supplier'][$s_template .'_supplier_street'] = array(
      '#type' => 'textfield',
      '#title' => t('Street'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($s_template, 'supplier_street', ''),    
    );
    $form[$s_template][$s_template .'_supplier'][$s_template .'_supplier_building_number'] = array(
      '#type' => 'textfield',
      '#title' => t('Building number'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($s_template, 'supplier_building_number', ''),    
    );
    $form[$s_template][$s_template .'_supplier'][$s_template .'_supplier_zipcode'] = array(
      '#type' => 'textfield',
      '#title' => t('Zipcode'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($s_template, 'supplier_zipcode', ''),
    );
    $form[$s_template][$s_template .'_supplier'][$s_template .'_supplier_city'] = array(
      '#type' => 'textfield',
      '#title' => t('City'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($s_template, 'supplier_city', ''),
    );
    $form[$s_template][$s_template .'_supplier'][$s_template .'_supplier_country'] = array(
      '#type' => 'textfield',
      '#title' => t('Country'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($s_template, 'supplier_country', ''),
    );
    $form[$s_template][$s_template .'_supplier'][$s_template .'_supplier_phone'] = array(
      '#type' => 'textfield',
      '#title' => t('Phone'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($s_template, 'supplier_phone', ''),
    );  
    $form[$s_template][$s_template .'_supplier'][$s_template .'_supplier_fax'] = array(
      '#type' => 'textfield',
      '#title' => t('Fax'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($s_template, 'supplier_fax', ''),
    );
    $form[$s_template][$s_template .'_supplier'][$s_template .'_supplier_email'] = array(
      '#type' => 'textfield',
      '#title' => t('Email'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($s_template, 'supplier_email', ''),
    );
    $form[$s_template][$s_template .'_supplier'][$s_template .'_supplier_web'] = array(
      '#type' => 'textfield',
      '#title' => t('Web address'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($s_template, 'supplier_web', ''),
    );
    $form[$s_template][$s_template .'_supplier'][$s_template .'_supplier_coc_number'] = array(
      '#type' => 'textfield',
      '#title' => t('CoC number'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($s_template, 'supplier_coc_number', ''),
    );
    $form[$s_template][$s_template .'_supplier'][$s_template .'_supplier_vat_number'] = array(
      '#type' => 'textfield',
      '#title' => t('VAT number'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($s_template, 'supplier_vat_number', ''),
    );
  }
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  
  return $form;
}

/**
 * Form helper function to get all invoice items
 * 
 * @param string $type
 * @param integer $invoice_id
 * 
 * @return array
 */
function _invoice_get_invoice_items($invoice_id=0, $type=NULL) {
  $i = 0;
  $invoice_id = intval($invoice_id);
  
  $invoice_items_table_rows = array();
  $sql_addition = $invoice_id > 0 ? ' '. intval($invoice_id) : '';
  
  $result = db_query("SELECT * FROM {invoice_items}
    WHERE uid=%d AND invoice_id=%d ORDER BY created ASC",
    $GLOBALS['user']->uid,
    $invoice_id
  );

  while ($row = db_fetch_object($result)) {
    $i++;
    $invoice_items_table_rows[] = array('data' => array(
      nl2br($row->description),
      $row->vat .'%',
      $row->quantity,
      money_format('%.3n', _invoice_round($row->unitcost, 3)),
      money_format('%.2n', _invoice_round($row->unitcost * ('1.'. $row->vat), 2)),
      money_format('%.2n', _invoice_round($row->quantity * $row->unitcost, 2)),
      money_format('%.2n', _invoice_round($row->quantity * $row->unitcost * ('1.'. $row->vat), 2)),
      array('data' =>
      _invoice_get_icon('edit', NULL, array(
        'onclick' => sprintf("invoice_edit_item('%d')", $row->iid),
        'class' => 'mouse-pointer',
        'title' => t('Edit'),
      ))
      . 
      _invoice_get_icon('delete', NULL, array(
        'onclick' => sprintf("invoice_delete_item('%d')", $row->iid),
        'class' => 'mouse-pointer',
        'title' => t('Delete'),
      )),
      'class' => 'actions'
      ),
     ),
     'class' => $type .'item-'. $row->iid,
    );
  }
  
  return array('rows' => $invoice_items_table_rows, 'count' => $i);
}

/**
 * Implementation of hook_form_alter()
 *
 * @param array $form
 * @param array $form_state
 * @param string $form_id
 */
function invoice_form_alter(&$form, $form_state, $form_id) {
  if ($form_id == 'invoice_node_form') {
    unset($form['buttons']['preview']);
  }
}