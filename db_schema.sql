--
-- PostgreSQL database dump
--

SET statement_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SET check_function_bodies = false;
SET client_min_messages = warning;

--
-- Name: plpgsql; Type: EXTENSION; Schema: -; Owner:
--

CREATE EXTENSION IF NOT EXISTS plpgsql WITH SCHEMA pg_catalog;


--
-- Name: EXTENSION plpgsql; Type: COMMENT; Schema: -; Owner:
--

COMMENT ON EXTENSION plpgsql IS 'PL/pgSQL procedural language';


SET search_path = public, pg_catalog;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: charge_controller_data; Type: TABLE; Schema: public; Owner: solar; Tablespace:
--

CREATE TABLE charge_controller_data (
    id bigint NOT NULL,
    cid smallint,
    ts timestamp without time zone DEFAULT timezone('utc'::text, now()),
    battery_volts real,
    pv_volts real,
    battery_volts_raw real,
    pv_volts_raw real,
    battery_amps real,
    pv_amps real,
    pv_voc real,
    watts smallint,
    kwh_today real,
    ah_today smallint,
    ext_temp real,
    int_fet_temp real,
    int_pcb_temp real,
    life_kwh real,
    life_ah integer,
    float_seconds_today integer,
    combochargestate integer,
    wbjr_soc integer,
    wbjr_remaining_ah integer
);

CREATE INDEX ON charge_controller_data(ts);

ALTER TABLE public.charge_controller_data OWNER TO solar;

--
-- Name: charge_controller_data_id_seq; Type: SEQUENCE; Schema: public; Owner: solar
--

CREATE SEQUENCE charge_controller_data_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.charge_controller_data_id_seq OWNER TO solar;

--
-- Name: charge_controller_data_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: solar
--

ALTER SEQUENCE charge_controller_data_id_seq OWNED BY charge_controller_data.id;


--
-- Name: charge_controller_list; Type: TABLE; Schema: public; Owner: solar; Tablespace:
--

CREATE TABLE charge_controller_list (
    cid integer NOT NULL,
    name character varying(64),
    smallname character(8),
    ip inet,
    mac macaddr,
    serial integer,
    deviceid character(9)
);


ALTER TABLE public.charge_controller_list OWNER TO solar;

--
-- Name: charge_controller_list_cid_seq; Type: SEQUENCE; Schema: public; Owner: solar
--

CREATE SEQUENCE charge_controller_list_cid_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.charge_controller_list_cid_seq OWNER TO solar;

--
-- Name: charge_controller_list_cid_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: solar
--

ALTER SEQUENCE charge_controller_list_cid_seq OWNED BY charge_controller_list.cid;


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: solar
--

ALTER TABLE ONLY charge_controller_data ALTER COLUMN id SET DEFAULT nextval('charge_controller_data_id_seq'::regclass);


--
-- Name: cid; Type: DEFAULT; Schema: public; Owner: solar
--

ALTER TABLE ONLY charge_controller_list ALTER COLUMN cid SET DEFAULT nextval('charge_controller_list_cid_seq'::regclass);


--
-- Name: charge_controller_data_pkey; Type: CONSTRAINT; Schema: public; Owner: solar; Tablespace:
--

ALTER TABLE ONLY charge_controller_data
    ADD CONSTRAINT charge_controller_data_pkey PRIMARY KEY (id);


--
-- Name: charge_controller_list_pkey; Type: CONSTRAINT; Schema: public; Owner: solar; Tablespace:
--

ALTER TABLE ONLY charge_controller_list
    ADD CONSTRAINT charge_controller_list_pkey PRIMARY KEY (cid);


--
-- Name: public; Type: ACL; Schema: -; Owner: postgres
--

REVOKE ALL ON SCHEMA public FROM PUBLIC;
REVOKE ALL ON SCHEMA public FROM postgres;
GRANT ALL ON SCHEMA public TO postgres;
GRANT ALL ON SCHEMA public TO PUBLIC;


--
-- PostgreSQL database dump complete
--
