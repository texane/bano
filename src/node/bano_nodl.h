#ifndef BANO_NODL_H_INCLUDED
#define BANO_NODL_H_INCLUDED


/* bano_nodl_cap_xxx */

enum bano_nodl_cap
{
  /* { plain, token, full, replay }, per message ? */
  BANO_NODL_CAP_SECURITY_SCHEME = 0,
  BANO_NODL_CAP_BANO_VERS,
  BANO_NODL_CAP_UPDATE_FREQ,
  BANO_NODL_CAP_MESSAGING_MODE,
  /* { nrf905, nrf24l01p } */
  BANO_NODL_CAP_RF_CHIP,
  BANO_NODL_CAP_RF_FREQ,
  BANO_NODL_CAP_RF_SPEED,
  BANO_NODL_CAP_INVALID
};


#endif /* BANO_NODL_H_INCLUDED */
