import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { StudioSection } from "../src/App";

const mockSetField = jest.fn();
const mockReset = jest.fn();
const mockRefresh = jest.fn();

jest.mock("../src/hooks/useCirqueState", () => {
  const { defaultCirqueState } = jest.requireActual<
    typeof import("../src/hooks/cirqueCodec")
  >("../src/hooks/cirqueCodec");
  return {
    useCirqueState: () => ({
      state: defaultCirqueState(),
      isConnected: true,
      isLoading: false,
      error: null,
      setField: mockSetField,
      reset: mockReset,
      refresh: mockRefresh,
    }),
  };
});

describe("StudioSection", () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  it("renders all 8 section cards", () => {
    render(<StudioSection />);

    for (const title of [
      "Mode & Sensitivity",
      "Axis",
      "Tap",
      "Edge Motion",
      "Edge Scroll",
      "Pointer",
      "Speed",
      "Misc",
    ]) {
      expect(screen.getByText(title)).toBeInTheDocument();
    }
  });

  it("prop-drills a Switch toggle to setField with the matching field", async () => {
    const user = userEvent.setup();
    render(<StudioSection />);

    await user.click(screen.getByRole("checkbox", { name: "Invert X" }));

    expect(mockSetField).toHaveBeenCalledWith("invertX", true, true);
  });

  it("wires the Reset and Refresh buttons to the hook", async () => {
    const user = userEvent.setup();
    render(<StudioSection />);

    await user.click(screen.getByRole("button", { name: "Reset to defaults" }));
    await user.click(screen.getByRole("button", { name: "Refresh" }));

    expect(mockReset).toHaveBeenCalledWith(true);
    expect(mockRefresh).toHaveBeenCalled();
  });
});
