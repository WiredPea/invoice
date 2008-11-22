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
 * Helper function to get invoice Javascript
 */
function _invoice_get_js() {
 return
 <<<EOD
$(document).ready(function(){    
  $("#edit-search").blur(function(){
    if ($("#edit-search").val().length > 0 && $("#edit-search").val() > 0) {
      invoice_get_customer_info($("#edit-search").val());
    }
    else if ($("#edit-search").val().length < 1) {
      invoice_get_customer_info('set_empty');
    }
  });
});

function invoice_set_template(value) {
  $.get("http://"+Drupal.settings['invoice']['host']+Drupal.settings['basePath']+"invoice/set/template",
    { value: value,
    },
    function(data) {    
      if (data['error'] != undefined && data['error'] != '') {        
        alert(data['error']);
      }
      else {
        $('#edit-vat').val(data['vat']);
      }
    },
    "json"
  );
}

function invoice_get_customer_info(value) {
  $.get("http://"+Drupal.settings['invoice']['host']+Drupal.settings['basePath']+"invoice/get/customer_info",
    { value: value,
    },
    function(data) {    
      if (data['error'] != undefined && data['error'] != '') {        
        alert(data['error']);
      }
      else if (value == 'set_empty' || (data['set_empty'] != undefined && data['set_empty'] != '')) {
        $('#edit-company-name').val('');
        $('#edit-firstname').val('');
        $('#edit-lastname').val('');
        $('#edit-street').val('');
        $('#edit-building-number').val('');
        $('#edit-zipcode').val('');
        $('#edit-city').val('');
        $('#edit-country').val('');
        $('#edit-customer-description').val('');
        $('#edit-coc-number').val('');
        $('#edit-vat-number').val('');
      }
      else {
        $('#edit-search').val(data['search_customer']);
        $('#edit-company-name').val(data['company_name']);
        $('#edit-firstname').val(data['firstname']);
        $('#edit-lastname').val(data['lastname']);
        $('#edit-street').val(data['street']);
        $('#edit-building-number').val(data['building_number']);
        $('#edit-zipcode').val(data['zipcode']);
        $('#edit-city').val(data['city']);
        $('#edit-country').val(data['country']);
        $('#edit-coc-number').val(data['coc_number']);
        $('#edit-vat-number').val(data['vat_number']);
        if (window.mceToggle) { mceToggle('edit-customer-description', 'wysiwyg4customer-description'); }
        $('#edit-customer-description').val(data['description']);
        if (window.mceToggle) { mceToggle('edit-customer-description', 'wysiwyg4customer-description'); }
      }
    },
    "json"
  );
}

function invoice_save_item(value) {
  $.post("http://"+Drupal.settings['invoice']['host']+Drupal.settings['basePath']+"invoice/save/item",
    { iid: $("#edit-iid").val(),
      invoice_number: $("#edit-invoice-number").val(),
      description: $("#edit-description").val(),
      quantity: $("#edit-quantity").val(),
      price_without_vat: $("#edit-price-without-vat").val(),
      price_with_vat: $("#edit-price-with-vat").val(),
      vat: $("#edit-vat").val(),
    },
    function(data) {    
      if (data['error'] != undefined && data['error'] != '') {        
        alert(data['error']);
      }
      else {
        if (data['remove_empty_row'] != undefined && data['remove_empty_row'] != '') {
          $('.invoice-items-empty').remove();
        }
                
        // if invoice item id is not empty we just changed an invoice item, so update the row
        if (data['iid'] != undefined && data['iid'] != '') {
          $('.invoice-items .item-'+data['iid']+' td:nth-child(1)').html(data['description']);
          $('.invoice-items .item-'+data['iid']+' td:nth-child(2)').html(data['vat']);
          $('.invoice-items .item-'+data['iid']+' td:nth-child(3)').html(data['quantity']);
          $('.invoice-items .item-'+data['iid']+' td:nth-child(4)').html(data['exunitcost']);
          $('.invoice-items .item-'+data['iid']+' td:nth-child(5)').html(data['incunitcost']);
          $('.invoice-items .item-'+data['iid']+' td:nth-child(6)').html(data['exsubtotal']);
          $('.invoice-items .item-'+data['iid']+' td:nth-child(7)').html(data['incsubtotal']);
        }
        else {
          $('.invoice-items table').append(data['content']);
        }

        // reset invoice item form
        $('#edit-iid').val('');
        $('#edit-description').val('');
        $('#edit-quantity').val('');
        $('#edit-price-without-vat').val('');
        $('#edit-price-with-vat').val('');
        $('#edit-vat').val(data['activevat']);
        $('#button-save-item').val(data['actionvalue']);
        
        // set new totals
        $('.invoice-items .extotal').html(data['extotal']);
        $('.invoice-items .inctotal').html(data['inctotal']);
      }
    },
    "json"
  );
}

function invoice_edit_item(value) {
  $.get("http://"+Drupal.settings['invoice']['host']+Drupal.settings['basePath']+"invoice/edit/item",
    { iid: value,
      invoice_number: $("#edit-invoice-number").val(),
    },
    function(data) {    
      if (data['error'] != undefined && data['error'] != '') {        
        alert(data['error']);
      }
      else {
        $('#edit-iid').val(value);
        $('#edit-description').val(data['description']);
        $('#edit-vat').val(data['vat']);
        $('#edit-quantity').val(data['quantity']);
        $('#edit-price-without-vat').val(data['exunitcost']);
        $('#edit-price-with-vat').val(data['incunitcost']);
        $('#button-save-item').val(data['actionvalue']);
      }
    },
    "json"
  );
}

function invoice_delete_item(value) {
  $.get("http://"+Drupal.settings['invoice']['host']+Drupal.settings['basePath']+"invoice/delete/item",
    { iid: value,
      invoice_number: $("#edit-invoice-number").val(),
    },
    function(data) {    
      if (data['error'] != undefined && data['error'] != '') {        
        alert(data['error']);
      }
      else {
        $('.invoice-items .item-'+value).remove();
        $('.invoice-items .extotal').html(data['extotal']);
        $('.invoice-items .inctotal').html(data['inctotal']);
      }
    },
    "json"
  );
}
EOD;
}

/**
 * Set the chosen invoice when creating a node
 */
function invoice_set_template() {
  $template = check_plain($_GET['value']);
  $_SESSION['invoice_template'] = $template;
  $a_items['vat'] = _invoice_get_variable($template, 'vat');
  drupal_json($a_items);
  exit();
}

/**
 * Search if the customer already exists
 */
function invoice_search_customer($value) {
  $a_items = array();
  $customer_value = check_plain($value);

  $result = db_query("SELECT * FROM {invoice_customers} ic
    LEFT JOIN {invoice_invoices} ii ON ic.invoice_id=ii.iid  
    WHERE ii.uid=%d AND (company_name LIKE '%%". $customer_value ."%' OR lastname LIKE '%%". $customer_value ."%' OR firstname LIKE '%%". $customer_value ."%')
    GROUP BY vat_number,coc_number,company_name,country,city,zipcode,building_number,lastname,firstname
    ORDER BY company_name, lastname, firstname, invoice_id DESC",
    $GLOBALS['user']->uid
  );
  while ($row = db_fetch_object($result)) {
    $key = $row->cid;
    $value = NULL;
    if (!empty($row->company_name)) {
      $value = $row->company_name;
      $value = !empty($row->zipcode) ? $value ." - ". $row->zipcode : $value;
      $value = !empty($row->building_number) ? $value ." - ". $row->building_number : $value;
      $value = !empty($row->city) ? $value ." - ". $row->city : $value;
      $value = !empty($row->country) ? $value ." - ". $row->country : $value;
      $value = !empty($row->vat_number) ? $value ." - ". $row->vat_number : $value;
      $value = !empty($row->coc_number) ? $value ." - ". $row->coc_number : $value;
      $value = !empty($row->lastname) ? $value ."\n    ". $row->lastname : $value;
      $value = !empty($row->firstname) ? $value ."\n    ". $row->firstname : $value;
    }
    else {
      $value = $row->lastname . (!empty($row->firstname) ? ', '. $row->firstname : '');
    }
    $a_items[$key] = check_plain($value); 
  }
  
  drupal_json($a_items);
  exit();
}

/**
 * Get customer info
 */
function invoice_get_customer_info() {
  // Create alias
  $fv =& $_GET;
  
  $a_data = db_fetch_array(db_query("SELECT company_name,firstname,lastname,street,building_number,zipcode,city,country,coc_number,vat_number,ic.description
    FROM {invoice_customers} ic
    LEFT JOIN {invoice_invoices} ii ON ic.invoice_id=ii.iid  
    WHERE ii.uid=%d AND ic.cid=%d
    ORDER BY company_name DESC, lastname DESC, firstname DESC, ic.invoice_id DESC
    LIMIT 1",
    $GLOBALS['user']->uid,
    $fv['value']
  ));
  
  if (count($a_data) == 0) {
    $a_data['set_empty'] = TRUE;
  }
  else {
    $a_data['search_customer'] = !empty($a_data['company_name']) ? $a_data['company_name'] : $a_data['lastname'] . (!empty($a_data['firstname']) ? ', '. $a_data['firstname'] : '');
  }
  
  drupal_json($a_data);
  exit;
}

/**
 * Add an invoice item
 */
function invoice_save_item() {
  // Create alias
  $fv =& $_POST;

  $a_data = array();

  // Set locale so money has the right format for the preferred culture
  if (intval($fv['invoice_number']) == 0) {
    if ($locale = _invoice_get_variable(_invoice_get_chosen_template(), 'locale')) {
      setlocale(LC_MONETARY, $locale);
    }
    $active_template = _invoice_get_chosen_template();
  }
  else {
    if ($template = db_result(db_query("SELECT it.name FROM {invoice_invoices} ii LEFT JOIN {invoice_templates} it ON ii.tid=it.tid WHERE ii.iid=%d", $fv['invoice_number']))) {
      if ($locale = _invoice_get_variable($template, 'locale')) {
        setlocale(LC_MONETARY, $locale);
      }
      $active_template = $template;
    }
  }
  
  // Display error if price_wihtout_vat and price_with_vat are both not filled in
  if (empty($fv['price_without_vat']) && empty($fv['price_with_vat'])) {
    $a_data['error'] = t('Error') .': '. t('You have to fill in either "Price without VAT" or "Price with VAT"!');  
  }
  
  if (!empty($a_data['error'])) {
    drupal_json($a_data);
    exit;
  }
  
  // Typecast strings to doubles and replace comma with a dot
  $fv['quantity'] = (double) str_replace(',', '.', $fv['quantity']);
  $fv['price_without_vat'] = (double) str_replace(',', '.', $fv['price_without_vat']);
  $fv['price_with_vat'] = (double) str_replace(',', '.', $fv['price_with_vat']);
  
  // Get the price without VAT
  if (!empty($fv['price_without_vat'])) {
    $unitcost = $fv['price_without_vat'];
  }
  else {
    $unitcost = $fv['price_with_vat'] / ('1.'. variable_get('invoice_vat', 0));
  }
  
  // Round the price to 3 decimals
  $unitcost = round($unitcost, 3);
  
  // Round quantity to 2 decimals
  $fv['quantity'] = round($fv['quantity'], 2);
  
  if (intval($fv['iid']) > 0) {
    // item id is greater than zero, so we are saving an existing invoice item
    db_query("UPDATE {invoice_items}
      SET description='%s', vat='%f', quantity='%f', unitcost='%f' WHERE iid=%d AND uid=%d AND invoice_id=%d",
      $fv['description'],
      $fv['vat'],
      $fv['quantity'],
      $unitcost,
      $fv['iid'],
      $GLOBALS['user']->uid,
      $fv['invoice_number']
    );
  }
  else {
    // Insert invoice item into the invoice items table
    db_query("INSERT INTO {invoice_items}
      (description,vat,quantity,unitcost,invoice_id,uid,created) VALUES ('%s',%f,%f,%f,%d,%d,'%s')",
      $fv['description'],
      $fv['vat'],
      $fv['quantity'],
      $unitcost,
      $fv['invoice_number'],
      $GLOBALS['user']->uid,
      date('Y-m-d H:i:s')
    );
  }

  // Count the added items and calculate invoice totals
  $count = db_result(db_query("SELECT COUNT(*) as count FROM {invoice_items} WHERE uid=%d AND invoice_id=%d",
    $GLOBALS['user']->uid,
    $fv['invoice_number']
  ));

  if (intval($fv['iid']) > 0) {
    // item id is greater than zero, so we are dealing with an existing invoice item
    $a_data['iid']          = check_plain($fv['iid']);
    $a_data['description']  = nl2br(check_plain($fv['description']));
    $a_data['vat']          = check_plain($fv['vat']) .'%';
    $a_data['quantity']     = check_plain($fv['quantity']);
    $a_data['exunitcost']   = money_format('%.3n', _invoice_round($unitcost, 3));
    $a_data['incunitcost']  = money_format('%.2n', _invoice_round($unitcost * ('1.'. $fv['vat']), 2));
    $a_data['exsubtotal']   = money_format('%.2n', _invoice_round($fv['quantity'] * $unitcost, 2));
    $a_data['incsubtotal']  = money_format('%.2n', _invoice_round($fv['quantity'] * $unitcost * ('1.'. $fv['vat']), 2));
    $a_data['actionvalue']  = t('Add item');
  }
  else {
    // Set row class name
    $class = 'item-'. db_last_insert_id('invoice_item', 'iid');
    
    // Compose content to send back to the browser
    $a_data['content'] = sprintf('<tr class="%s"><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td class="actions">%s</td></tr>',
      $count & 1 ? $class .' odd' : $class .' even',
      nl2br(check_plain($fv['description'])),
      check_plain($fv['vat']) .'%',
      check_plain($fv['quantity']),
      money_format('%.3n', _invoice_round($unitcost, 3)),
      money_format('%.2n', _invoice_round($unitcost * ('1.'. $fv['vat']), 2)),
      money_format('%.2n', _invoice_round($fv['quantity'] * $unitcost, 2)),
      money_format('%.2n', _invoice_round($fv['quantity'] * $unitcost * ('1.'. $fv['vat']), 3)),
      _invoice_get_icon('edit', NULL, array(
        'onclick' => sprintf("invoice_edit_item('%d')", db_last_insert_id('invoice_item', 'iid')),
        'class' => 'mouse-pointer',
        'title' => t('Edit'),
      ))
      . 
      _invoice_get_icon('delete', NULL, array(
        'onclick' => sprintf("invoice_delete_item('%d')", db_last_insert_id('invoice_item', 'iid')),
        'class' => 'mouse-pointer',
        'title' => t('Delete'),
      ))
    );
  
    // Remove the empty from the page
    $a_data['remove_empty_row'] = TRUE;
  }
  
  // Define active vat value to put back on the resetted item form
  $a_data['activevat'] = _invoice_get_variable($active_template, 'vat');
  
  // Get invoice totals
  $a_totals = _invoice_get_invoice_totals($fv['invoice_number'], $GLOBALS['user']->uid);
  
  // Set total
  $a_data['extotal'] = money_format('%.2n', _invoice_round($a_totals['extotal'], 2));
  $a_data['inctotal'] = money_format('%.2n', _invoice_round($a_totals['inctotal'], 2));

  drupal_json($a_data);
  exit;
}

/**
 * Edit an invoice item
 */
function invoice_edit_item() {
  $fv =& $_GET;
  $a_data = array();
  
  // Check if the item to delete exists and is owned by this owner
  $a_invoice = db_fetch_object(db_query("SELECT *, COUNT(*) AS count
    FROM {invoice_items}
    WHERE iid=%d AND uid=%d AND invoice_id=%d GROUP BY iid",
    $fv['iid'],
    $GLOBALS['user']->uid,
    $fv['invoice_number']
  ));

  if ($a_invoice->count == 0) {
    $a_data['error'] = t('This item id does not exist or you are not the owner!');
  }
  else {
    $a_data['description'] = $a_invoice->description;
    $a_data['vat'] = $a_invoice->vat;
    $a_data['quantity'] = $a_invoice->quantity;
    $a_data['exunitcost'] = $a_invoice->unitcost;
    $a_data['incunitcost'] = $a_invoice->unitcost * ('1.'. variable_get('invoice_vat', 0));
    $a_data['actionvalue']  = t('Save item');
  }
  
  drupal_json($a_data);
  exit;
}

/**
 * Delete an invoice item
 */
function invoice_delete_item() {
  $fv =& $_GET;
  $a_data = array();
 
  // Set locale so money has the right format for the preferred culture
  if (intval($fv['invoice_number']) == 0) {
    if ($locale = _invoice_get_variable(_invoice_get_chosen_template(), 'locale')) {
      setlocale(LC_MONETARY, $locale);
    }
  }
  elseif ($template = db_result(db_query("SELECT it.name FROM {invoice_invoices} ii LEFT JOIN {invoice_templates} it ON ii.tid=it.tid WHERE ii.iid=%d", $fv['invoice_number']))) {
    if ($locale = _invoice_get_variable($template, 'locale')) {
      setlocale(LC_MONETARY, $locale);
    }
  }
  
  // Check if the item to delete exists and is owned by this owner
  $a_invoice = db_fetch_object(db_query("SELECT COUNT(*) AS count FROM {invoice_items} WHERE iid=%d AND uid=%d AND invoice_id=%d GROUP BY iid",
    $fv['iid'],
    $GLOBALS['user']->uid,
    $fv['invoice_number']
  ));

  if ($a_invoice->count == 0) {
    $a_data['error'] = t('This item id does not exist, does not belong to this invoice or you are not the owner!');
  }
  else {
    db_query("DELETE FROM {invoice_items} WHERE iid=%d AND uid=%d AND invoice_id=%d",
      $fv['iid'],
      $GLOBALS['user']->uid,
      $fv['invoice_number']
    );
    
    // Get invoice totals
    $a_totals = _invoice_get_invoice_totals($fv['invoice_number'], $GLOBALS['user']->uid);

    // Set total
    $a_data['extotal'] = money_format('%.2n', _invoice_round($a_totals['extotal'], 2));
    $a_data['inctotal'] = money_format('%.2n', _invoice_round($a_totals['inctotal'], 2));
  }
   
  drupal_json($a_data);
  exit;
}