#include <boost/format.hpp>
#include "Core.hpp"

#include "Controller/Iop/Sio2/CSio2.hpp"

#include "Resources/RResources.hpp"

CSio2::CSio2(Core * core) : 
	CController(core)
{
}

void CSio2::handle_event(const ControllerEvent & event) const
{
	switch (event.type)
	{
	case ControllerEvent::Type::Time:
	{
		int ticks_remaining = time_to_ticks(event.data.time_us);
		while (ticks_remaining > 0)
			ticks_remaining -= time_step(ticks_remaining);
		break;
	}
	default:
	{
		throw std::runtime_error("CSio2 event handler not implemented - please fix!");
	}
	}
}

int CSio2::time_to_ticks(const double time_us) const
{
	int ticks = static_cast<int>(time_us / 1.0e6 * Constants::IOP::SIO2::SIO2_CLK_SPEED * core->get_options().system_biases[ControllerType::Type::Sio2]);
	
	if (ticks < 10)
	{
		static bool warned = false;
		if (!warned)
		{
			BOOST_LOG(Core::get_logger()) << "Sio0 ticks too low - increase time delta";
			warned = true;
		}
	}

	return ticks;
}

int CSio2::time_step(const int ticks_available) const
{
	auto& r = core->get_resources();
	auto& ctrl = r.iop.sio2.ctrl;
	auto& intc_stat = r.iop.intc.stat;

    auto _ctrl_lock = ctrl.scope_lock();
	
	if (ctrl.write_latch)
	{
		if (ctrl.extract_field(Sio2Register_Ctrl::RESET_DIR) > 0)
		{
            // Receive packet. Value should be 0x3BD.
			if (ctrl.read_uword() != 0x3BD)
				BOOST_LOG(Core::get_logger()) << 
					boost::format("Careful, SIO2 ctrl recv value not normal: 0x%08X") 
					% ctrl.read_uword();

		    // Raise IOP IRQ.
		    intc_stat.insert_field(IopIntcRegister_Stat::SIO2, 1);

			// Clear the direction bit (no idea why... seems to be required).
			ctrl.insert_field(Sio2Register_Ctrl::RESET_DIR, 0);
		}
		else
		{
            // Send packet. Value should be 0x3BC.
			if (ctrl.read_uword() != 0x3BC)
				BOOST_LOG(Core::get_logger()) << 
					boost::format("Careful, SIO2 ctrl send value not normal: 0x%08X") 
					% ctrl.read_uword();
		}
	
		ctrl.write_latch = false; 
	}

	return 1;
}